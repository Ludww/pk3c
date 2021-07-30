/* Pacing Kernel 3 Tcp congestion control algorithm (PK3C 0.908) that is originally based on "Performance-oriented Congestion Control (PCC)" (GPLv2/BSD):
 *
 * that is rate-based congestion control algorithm that chooses its sending rate
 * based on an explicit utility function computed over rtt-length intervals and
 * on throughput (calculated speed of packets acked/receive) and on few other ideas:
 * 1. Calculated throughput becomes much lower than real when coming close to congestion (means can calculate correctly throughput only when optimal rate);
 * 2. Rtt often non reliable for Kernel 3 (so calculating avg rtt in different way than usually for skipping max/min incorrect values);
 * 3. The utility function (taken originally from PCC) based on RTT and Lose works Ok only for rates higher than 25000 (so
 *    this current version of PK3C doesn't work for less than 250KBits/sec links, but can think to use later regular Cubic or Reno when < 25000)
 *    - the reason that original PCC does every "experiment" (originally one "experiment" is 50 packets measurement, but later upgraded, see below)
 *    and then measures results after this experiment (looking the way how utility function changes or simpler to say of amount of loss and rtt increases or decreases),
 *    but for low rates original PCC cannot see input signals, because of noisy being stronger than such signal (and both reaction to previous experiment too slow for seeing it
 *    immediatelly), so the original idea by PCC can work good only with fast enough connections;
 *    second problem (apart that original PCC doesn't work good with low rates) is that it doesn't work at all for low latency
 *    (if latency less than about 5ms, means ping gives 4/1000sec or less, then utility function gives something random and as result PCC doesn't converge);
 *    - however, did some experiments with addition of minrate/maxrate (exists in this PK3C) and addition of random changes UP/DOWN (commented in this source with
 *    marks "POINT") and with these additions together PK3C starts working reasonably with very low latencies too;
 * 4. The amount of loss calculated differently comparing to original PCC (the idea is to calculate it using info about current sending buffer
 *    and info about acked packets for marking packets as loss if both exists in resend buffer and both hole in acked, but before RTO happened,
 *    so before socket value "lost-out" increased);
 * 5. Non obvious part of PK3C is slow-start: 
 *    a. Originally PCC did double rate during slow-start until loss detected (similar like Cubic does), but for some cases like local direct wire connections
 *       loss doesn't happen at all (for example because flow control buffers too small and max pacing rate then controlled by receiver), so need some
 *       additional condition to stop slow-start (the addition condition is HANDBRAKE for comparing current rate with thoughput and if current
 *       rate 3 times higher than throughput or throughput started decrese, then stop slow-start, and both for making it work during slow start similar
 *       way for all connections, for regular end of slow-start try to stop with throughput*3/2 similar way like of SLOW DEBUG start handbrake);
 *       The formula to calculate utility func during slow start is
 *       "util = rate - (rate * (loss_penalty * loss_ratio)) / PCC_SCALE;" (see in code below)
 *       and loss_penalty is const==25;
 *       and original PCC stop slow-start when next calculated util less than prev;
 *       also tried to use rtt inside this formula too some time before, but it didn't help at all (mostly because of noise when rtt can change both directions
 *       both before optimal rate found during slow-start and after);
 *       so for PK3C using both loss-based slow-start stop (similar to PCC) and additional Handbrake (that is required for very low rates like 25000 or lower);
 *    b. The other problem of original PCC is that it takes too much time during slow-start (like it goes too high before it understand that need to stop slow-start),
 *       so first in PK3C did decrease amount of packets during slow-start (can be about 15, see const MIN_PACKETS_PER_INTERVAL, per interval when slow-start) and
 *       second start from high rate as first step of slow-start (like rate == PCC_RATE_MIN * 512 * 6 that is about 300000 that is ~3MBs/sec),
 *       and as result slow-start (for rates from 256KBits to very high like 10Gb/s) finishes the same fast (or faster) like Cubic does. And no need
 *       to wait about 20 seconds during slow-start like in original PCC was.
 * 6. The original PCC tried to work the same way both for high rates and low rates connections.
 *       As result, the higher rate connections utilized all the channel and the lower rate worked few times slower, and for solving this issue:
 *    a. Added fast-moving mode (so low-rate connection knows that it is low-rate and increases rate very fast with smallest few packets intervals
 *       until some condition);
 *    b. Did some differentiation in amount of packets per interval (so for high speed connection can be 300 packets per interval and for lower rate 50 packets
 *       and if slow-start or moving-fast, then 15 packets for slow-start or even lower amount of packets than 15 for moving-fast).
 */


#include <linux/version.h>

#include <linux/math64.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <net/tcp.h>


#include <linux/types.h>
#include <linux/atomic.h>

#include <linux/moduleparam.h>
#include <linux/delay.h>

#include "tcp_pk3c.h"
#include "commdefs.h"

#include "proc_commonconsts.h"
#include "proc_interface.h"
#include "pk3c_proc_version.h"


#define PK3C_PROBING_EPS 5
#define PK3C_PROBING_EPS_PART 100

#define PK3C_SCALE 1000

#define PK3C_RATE_MIN 128ull // in PK3C was 1024u instead

#define PK3C_RATE_MIN_PACKETS_PER_RTT 2
#define PK3C_INVALID_INTERVAL -1
#define PK3C_IGNORE_PACKETS 10
#define PK3C_INTERVAL_MIN_PACKETS 20
#define PK3C_MIN_INTERVALS_BEFORE_UPDOWN_HANDBRAKE 40


#define MIN_PACKETS_PER_INTERVAL 15
#define PK3C_ALPHA 100

#define PK3C_GRAD_STEP_SIZE 100
#define PK3C_MAX_SWING_BUFFER 2

#define PK3C_LAT_INFL_FILTER 10

#define PK3C_MIN_RATE_DIFF_RATIO_FOR_GRAD 20

#define PK3C_MIN_CHANGE_BOUND 50
#define PK3C_CHANGE_BOUND_STEP 1
#define PK3C_AMP_MIN 2

#define USE_PROBING

#define NUM_OF_PCKTS_FOR_AVG 10


#define MAX_RATE_LIMIT 128000000LL
#define MIN_RATE_LIMIT 25000LL

#define MIN_X_RELIABLE_VALUE 15
#define THE_T_VALUE 25

enum PK3C_LAST_CALL_NAMES {
	LCALL_UNKNOWN,
	LCALL_INIT,
	LCALL_STATE,
	LCALL_PROCESS,
	LCALL_FINISH,
};


static int log_level = 7; // KERN_DEBUG
static int agent_communication = 1; 
static int loss_penalty = 25; 
static int rtt_penalty = 25;
static int slow_start_start = PK3C_RATE_MIN * 512 * 6;

module_param(log_level,int,0);
MODULE_PARM_DESC(log_level, "logging level; 7 (default, KERN_DEBUG) - log everything, 0 - log nothing");
module_param(agent_communication,int,0);
MODULE_PARM_DESC(agent_communication, "send messages to agent 1 (default) - yes, 0 - no");
module_param(loss_penalty,int,0);
MODULE_PARM_DESC(loss_penalty, "penalty for loss occured");
module_param(rtt_penalty,int,0);
MODULE_PARM_DESC(rtt_penalty, "penalty for RTT inflation");
module_param(slow_start_start,int,0);
MODULE_PARM_DESC(slow_start_start, "slow start inital rate");

static atomic_t id_ = ATOMIC_INIT(0);
static atomic_t con_cnt_ = ATOMIC_INIT(0);


static unsigned int bufsize __read_mostly = 256;




static struct {
	spinlock_t	lock;
	wait_queue_head_t wait;

	unsigned long	head, tail;
	struct tcp_log	*log;
} tcp_probe_pk3c;



int cprintk(const char *buffer, ...) 
{
	va_list args;
	int result = 0;

	if (buffer[0] == KERN_SOH_ASCII && buffer[1]) {
		switch (buffer[1]) {
			case '0' ... '7':
				if(buffer[1] -'0' <= log_level) {
					va_start(args, buffer);
					result = vprintk(buffer, args);
					va_end(args);
				}

		}
	}

	return result;
}


static inline u64 ktime_u64(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
	return ktime_get_real().tv64;
#else
	return ktime_get_real();
#endif
}




static u32 pk3c_get_rtt(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);


#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
	struct pk3c_data *pk3c = inet_csk_ca(sk);

	if (pk3c && pk3c->lrtt) {
		return max(pk3c->lrtt, 1U);
#else
	if (tp->srtt_us) {
		return max(tp->srtt_us >> 3, 1U);
#endif
	} else {
		return USEC_PER_MSEC;
	}
}


static void pk3c_set_cwnd(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);


        u64 cwnd = sk->sk_pacing_rate;

        cwnd *= pk3c_get_rtt(sk);

        cwnd /= tp->mss_cache;

	cwnd /= USEC_PER_SEC;
	cwnd *= 2;

        cwnd = max(4ULL, cwnd);
	cwnd = min((u32)cwnd, tp->snd_cwnd_clamp);


        tp->snd_cwnd = cwnd;
}


bool pk3c_valid(struct pk3c_data *pk3c)
{
	return (pk3c && pk3c->intervals && pk3c->intervals[0].rate);
}


static void pk3c_setup_intervals_probing(struct pk3c_data *pk3c)
{

	s64 rate_low, rate_high;
	char rand;
	int i;

	get_random_bytes(&rand, 1);
	rate_high = pk3c->nums->rate * (PK3C_PROBING_EPS_PART + PK3C_PROBING_EPS * pk3c->nums->probing_cnt );
	rate_low = pk3c->nums->rate * (PK3C_PROBING_EPS_PART - PK3C_PROBING_EPS * pk3c->nums->probing_cnt );

	rate_high /= PK3C_PROBING_EPS_PART;
	rate_low /= PK3C_PROBING_EPS_PART;

	if(rate_low < MIN_RATE_LIMIT/2)
		rate_low = MIN_RATE_LIMIT/2;
	if(rate_high > MAX_RATE_LIMIT)
		rate_high = MAX_RATE_LIMIT;



	for (i = 0; i < PK3C_INTERVALS; i += 2) {
		if ((rand >> (i / 2)) & 1) {
			pk3c->intervals[i].rate = rate_low;
			pk3c->intervals[i + 1].rate = rate_high;
		} else {
			pk3c->intervals[i].rate = rate_high;
			pk3c->intervals[i + 1].rate = rate_low;
		}

		pk3c->intervals[i].packets_sent_base = 0;
		pk3c->intervals[i + 1].packets_sent_base = 0;
	
		pk3c->intervals[i].total_snt = 0;
		pk3c->intervals[i + 1].total_snt = 0;
	}

	pk3c->send_index = 0;
	pk3c->receive_index = 0;
	pk3c->wait = false;
	cprintk(KERN_DEBUG "setup_intervals_probing, so set pk3c->wait = false\n");
}


 static void pk3c_setup_intervals_moving(struct pk3c_data *pk3c, u32 bytes_acked)
{
	pk3c->intervals[0].packets_sent_base = 0;
	pk3c->intervals[0].min_rtt = BIG_VAL_S32;
	pk3c->intervals[0].max_rtt = 0;

	pk3c->intervals[0].rate = pk3c->nums->rate;
	pk3c->send_index = 0;
	pk3c->receive_index = 0;
	pk3c->wait = false;
}


static void start_interval(struct sock *sk, struct pk3c_data *pk3c)
{	
	u64 rate;
	struct tcp_sock *tp = tcp_sk(sk);
	bool hb = false;
	struct pk3c_interval *interval;

	rate = pk3c->nums->rate;

	if(pk3c->start_mode && pk3c->nums->thr_prev && pk3c->nums->thr_prev > rate / 10000 &&
	   pk3c->nums->thr_prev < rate / 3 &&
	   pk3c->nums->thr_prev * 5 >= MIN_RATE_LIMIT &&
	   pk3c->nums->thr_prev <= MAX_RATE_LIMIT
	  ) {
		rate = pk3c->nums->thr_prev;
		if(pk3c->nums->thr > pk3c->nums->thr_prev)
			rate = pk3c->nums->thr;

		if(rate < MIN_RATE_LIMIT) {
			rate = MIN_RATE_LIMIT;
			pk3c->nums->rate = rate;
		}
		
		hb = true;
		cprintk(KERN_DEBUG "%i HANDBRAKE DURING SLOWSTART: reinit rate from %llu to %llu based on thr_prev %llu\n", pk3c->nums->id, pk3c->nums->rate, rate, pk3c->nums->thr_prev);
		pk3c->nums->rate = rate;
		pk3c->nums->handbrake = rate;
	}

	if (!pk3c->wait) {
		if(pk3c->start_mode)
			cprintk(KERN_DEBUG "Start mode and !pk3c->wait\n");
		interval = &pk3c->intervals[pk3c->send_index];
		interval->packets_ended = 0;
		interval->lost = 0;
		interval->delivered = 0;
		interval->cnt_min_pkts = 0;
		interval->min_rtt = BIG_VAL_S32;
		interval->max_rtt = 0;

		interval->packets_sent_base = pk3c->nums->pcktscnt;
		interval->packets_out_base = tcp_sk(sk)->packets_out;

		interval->packets_sent_base = max(interval->packets_sent_base, 1U);
		interval->send_start = ktime_u64()/1000;
		interval->send_end = 0;

		if(!hb)
			rate = interval->rate;

		interval->recv_start = 0;
		interval->recv_end = 0;
	}

	rate = max(rate, MIN_RATE_LIMIT);
	

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	rate = min(rate, (u64)sk->sk_max_pacing_rate);

	sk->sk_pacing_rate = rate;
	pk3c_set_cwnd(sk);
	cprintk(KERN_DEBUG "kernel 4.9+: id=%i set rate to %llu minrate %llu maxrate %llu\n", pk3c->nums->id, rate, pk3c->nums->minrate, pk3c->nums->maxrate);
#else
	cprintk(KERN_DEBUG "kernel 3.x: id=%i set rate to %llu minrate %llu maxrate %llu\n", pk3c->nums->id, rate, pk3c->nums->minrate, pk3c->nums->maxrate);

	sk->sk_pacing_rate = rate;
	sk->sk_max_pacing_rate = rate;

	pk3c_set_cwnd(sk);
#endif

	if(pk3c->nums->minrate + pk3c->nums->minrate / 300 < pk3c->nums->maxrate / 2)
		pk3c->nums->minrate += pk3c->nums->minrate / 300;
	if(pk3c->nums->maxrate - pk3c->nums->maxrate / 10 > pk3c->nums->minrate * 2)
		pk3c->nums->maxrate -= pk3c->nums->maxrate / 10;
	if(rate < pk3c->nums->minrate)
		pk3c->nums->minrate = rate;
	if(rate > pk3c->nums->maxrate)
		pk3c->nums->maxrate = rate;

}


 
#define PK3C_LOSS_MARGIN 5
#define PK3C_MAX_LOSS 10

static u32 pk3c_exp(s32 x)
{
	s64 temp = PK3C_SCALE;
	s64 e = PK3C_SCALE;
	int i;

	for (i = 1; temp != 0; i++) {
		temp *= x;
		temp /= i;
		temp /= PK3C_SCALE;
		e += temp;
	}
	return e;
}

 
static s64 pk3c_calc_util_grad(s64 rate_1, s64 util_1, s64 rate_2, s64 util_2) {
	s64 rate_diff_ratio = (PK3C_SCALE * (rate_2 - rate_1)) / rate_1;
	if (rate_diff_ratio < PK3C_MIN_RATE_DIFF_RATIO_FOR_GRAD &&
		rate_diff_ratio > -1 * PK3C_MIN_RATE_DIFF_RATIO_FOR_GRAD)
		return 0;

	return (PK3C_SCALE * PK3C_SCALE * (util_2 - util_1)) / (rate_2 - rate_1);
}


static void init_monitor(struct sock *sk, struct pk3c_data *pk3c)
{
	struct tcp_sock *tp = tcp_sk(sk);
	pk3c->monitor_lost->valid = 0;
	pk3c->monitor_lost->start_time = ktime_u64();
	pk3c->monitor_lost->snd_start_seq = tp->snd_nxt;
	pk3c->monitor_lost->snd_end_seq = 0;
	if(!pk3c->monitor_lost->last_acked_seq)
		pk3c->monitor_lost->last_acked_seq = tp->snd_nxt;
	pk3c->monitor_lost->bytes_lost = 0;
}


static u64 calc_T_formula(u64 rate, u32 lrtt, struct sock *sk)
{
	s64 res;
	struct tcp_sock *tp = tcp_sk(sk);
	res = rate/tp->mss_cache*(u64)lrtt/1000000;
	if(res < 1)
		res = 1;
	if(res > 500)
		res = 500;
	return res;
}

static void pk3c_calc_utility_vivace(struct pk3c_data *pk3c, struct pk3c_interval *interval, struct sock *sk) {
        s64 loss_ratio, delivered, received, lost, snt, mss, rate, throughput, util;
	s64 lat_infl = 0;
	s64 rtt_diff;
	s64 rtt_diff_thresh = 0;
	s64 send_dur;
	s64 recv_dur;
	s64 recv_bytes;
	int vals;
	bool handbrake = false;

	snt = (interval->packets_ended - interval->packets_sent_base); 
	rtt_diff = 0;

	recv_dur = interval->recv_end - interval->recv_start;
	recv_bytes = interval->recv_end_acked_bytes - interval->recv_start_acked_bytes;
	send_dur = interval->send_end - interval->send_start;

	lost = interval->lost;
	delivered = interval->delivered;
	mss = tcp_sk(sk)->mss_cache;
	received = recv_bytes / mss;
	if(received > delivered)
		received = delivered;
	rate = interval->rate;
	throughput = 0;

	if (recv_dur > 0 && recv_bytes > 0) {
		throughput = (USEC_PER_SEC * (recv_bytes) ) / recv_dur;
		if(throughput < MIN_RATE_LIMIT)
			throughput = rate / 2;

		if(!pk3c->nums->thr_prev || !pk3c->nums->lost || pk3c->nums->thr_prev_cnt<2) {
			if(pk3c->nums->thr > pk3c->nums->thr_prev || pk3c->nums->thr_prev_cnt > 10)
				pk3c->nums->thr_prev = pk3c->nums->thr;
			if(pk3c->nums->thr_prev_cnt < 2 && throughput > pk3c->nums->thr_prev)
				pk3c->nums->thr_prev = throughput;
			pk3c->nums->thr_prev_cnt++;
		}
		pk3c->nums->thr = throughput;
		pk3c->nums->lost = lost; 
	} else {
		throughput = 1;
	}
	
	if (delivered == 0) {
		cprintk(KERN_DEBUG "No packets delivered\n");
		interval->utility = -BIG_VAL_S32;
		return;
	}

	interval->avg_start_rtt = 0;
	interval->avg_end_rtt = 0;
	vals = 0;

	if(interval->cnt_min_pkts > 0) {
		for(vals = 0; vals < interval->cnt_min_pkts && vals < NUM_OF_PCKTS_FOR_AVG; vals++) {
			interval->avg_start_rtt += interval->start_rtt[vals];
			interval->avg_end_rtt += interval->end_rtt[vals];
		}
	}

	if(vals > 0) {
		interval->avg_start_rtt /= vals;
		interval->avg_end_rtt /= vals;
	}


	if(pk3c->start_mode && pk3c->nums->slow_start_prev_rtt) {
		rtt_diff = interval->avg_end_rtt - pk3c->nums->slow_start_prev_rtt;
	}
	else if(pk3c->start_mode)
		rtt_diff = 0;
	else
		rtt_diff = interval->avg_end_rtt - interval->avg_start_rtt;


	if(!pk3c->nums->handbrake) {
	
	if(lost > 0)
		pk3c->nums->lostcnt = 10;
	else
		if(pk3c->nums->lostcnt)
			pk3c->nums->lostcnt--;

        // POINTB
	if(!pk3c->start_mode && lost && !pk3c->nums->cnt_no_lost_intervals &&
	   !pk3c->moving_fast &&
	   snt > 12
	  ) { 
		if(lost > snt/20) {
			handbrake = true;

			if(rate >= MIN_RATE_LIMIT) {
				if(pk3c->nums->maxrate && pk3c->nums->rate > (pk3c->nums->maxrate+pk3c->nums->minrate)/2 - (pk3c->nums->maxrate+pk3c->nums->minrate)/4 )
					pk3c->nums->handbrake = pk3c->nums->rate * 3 / 4;
				else {
					pk3c->nums->handbrake = pk3c->nums->rate * 8 / 9;
				}
			} else
			cprintk(KERN_DEBUG "%i new DEBUG HANDBRAKE (also based on %llu > 10) inited handbrake in _vivace, based lost=%llu > 10%%=%llu (updated rate from %llu to %llu)\n", pk3c->nums->id, calc_T_formula(rate, pk3c->lrtt, sk), lost, snt, pk3c->nums->rate, pk3c->nums->handbrake);
			pk3c->nums->cnt_no_lost_intervals = PK3C_MIN_INTERVALS_BEFORE_UPDOWN_HANDBRAKE;
			if(pk3c->nums->maxrate && rate < (pk3c->nums->maxrate+pk3c->nums->minrate)/2 - (pk3c->nums->maxrate+pk3c->nums->minrate)/4 )
				pk3c->nums->cnt_no_lost_intervals /= 10;


			if(pk3c->nums->handbrake && pk3c->nums->handbrake < MIN_RATE_LIMIT)
				pk3c->nums->handbrake = MIN_RATE_LIMIT;

		}
	}
	if(pk3c->nums->cnt_no_lost_intervals)
		pk3c->nums->cnt_no_lost_intervals--;

	}
	interval->cnt_min_pkts = 0;


#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
	if (throughput > 0)
		rtt_diff_thresh = (2 * mss) / throughput;
	if (send_dur > 0)
		lat_infl = (PK3C_SCALE * rtt_diff) / send_dur;
#else
	if (throughput > 0)
		rtt_diff_thresh = (2 * USEC_PER_SEC * mss) / throughput;
	if (send_dur > 0) {
		lat_infl = (PK3C_SCALE * rtt_diff) / send_dur;
	}
#endif

	pk3c->nums->slow_start_prev_rtt = interval->avg_start_rtt;

	if (lat_infl < PK3C_LAT_INFL_FILTER && lat_infl > -1 * PK3C_LAT_INFL_FILTER) {
		lat_infl = 0;
	}

	if (lat_infl < 0 && pk3c->start_mode) {
		lat_infl = 0;
	}

	loss_ratio = (lost * PK3C_SCALE) / (lost + snt);

	if (pk3c->start_mode && loss_ratio < 100)
		loss_ratio = 0;

	if(!pk3c->nums->thr && rate)
		pk3c->nums->thr = rate;


	if(pk3c->start_mode) {
		util = rate - (rate * (loss_penalty * loss_ratio)) / PK3C_SCALE;
	} else {
		if(pk3c->moving_fast)
			util = rate - rate * (rtt_penalty * lat_infl + loss_penalty * loss_ratio) / PK3C_SCALE;
		else
			util = pk3c->nums->thr - pk3c->nums->thr * (rtt_penalty * lat_infl + loss_penalty * loss_ratio) / PK3C_SCALE;
	}

	cprintk(KERN_INFO
	       "%d ucalc: rate %lld sent %u delv %lld ~recv %lld lost %lld lat (%lld->%lld) util %lld thpt %lld\n",
	       pk3c->nums->id, rate, snt,
	       delivered, received, lost, (interval->avg_start_rtt / USEC_PER_MSEC), (interval->avg_end_rtt / USEC_PER_MSEC),
	       util, throughput);

	if(!pk3c->nums->handbrake) {
	    if(handbrake) {
		interval->utility = 0;
		if(!pk3c->nums->handbrake) {
			if(pk3c->nums->rate >= MIN_RATE_LIMIT) {
				pk3c->nums->handbrake = pk3c->nums->rate;
			}
 			else {
				pk3c->nums->handbrake = MIN_RATE_LIMIT;
			}
		}
	    }
	}

	interval->utility = util;
}

static enum PK3C_DECISION pk3c_get_decision(struct pk3c_data *pk3c, u32 new_rate)
{
	if (new_rate >= MAX_RATE_LIMIT)
		return PK3C_RATE_DOWN;

	if (pk3c->nums->rate == new_rate)
		return PK3C_RATE_STAY;

	return pk3c->nums->rate < new_rate ? PK3C_RATE_UP : PK3C_RATE_DOWN;
}

static u32 pk3c_decide_rate(struct pk3c_data *pk3c)
{
	bool run_1_res_, run_2_res_, did_agree_;
	int i2 = 2;
	int i3 = 3;
	u32 ret;


	if(pk3c->intervals[0].rate != pk3c->intervals[2].rate) {
		i3 = 2;
		i2 = 3;
	}

	run_1_res_ = pk3c->intervals[0].utility > pk3c->intervals[1].utility;
	run_2_res_ = pk3c->intervals[i2].utility > pk3c->intervals[i3].utility;

	did_agree_ = run_1_res_ == run_2_res_;


	if(did_agree_) {
		if (run_2_res_) {
			pk3c->intervals[0].utility = pk3c->intervals[i2].utility;
	
			ret = pk3c->intervals[i2].rate;
		} else {
			pk3c->intervals[0].utility = pk3c->intervals[i3].utility;
	
			ret = pk3c->intervals[i3].rate;
		}
	} else {
		ret = pk3c->nums->rate;
	}


	return ret;
}

static void pk3c_decide(struct pk3c_data *pk3c, struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct pk3c_interval *interval;
	u32 new_rate;
	int i;
	bool can_enter_fastmoving_based_on_throughput;
	u32 total_snt;
	u64 curtime;
	u32 total_packets_lost = 0;
	u32 total_delivered = 0;
	s64 total_utility = 0;
	s64 total_rate = 0;
	u32 min_rtt = BIG_VAL_S32;
	u32 max_rtt = 0;
	u32 total_avg_rtt = 0;
	u32 total_lost = 0;



	for (i = 0; i < PK3C_INTERVALS; i++ ) {
		interval = &pk3c->intervals[i];
		(*pk3c->util_func)(pk3c, interval, sk);


		total_packets_lost += interval->lost;
		total_delivered += interval->delivered;
		total_utility += interval->utility;
		total_avg_rtt += (pk3c->intervals[i].avg_start_rtt + pk3c->intervals[i].avg_end_rtt)/2;
		total_rate += interval->rate;
		total_lost += interval->lost;

		if(pk3c->intervals[i].max_rtt > max_rtt)
		        max_rtt = pk3c->intervals[i].max_rtt;
		if(pk3c->intervals[i].min_rtt < min_rtt)
                        min_rtt = pk3c->intervals[i].min_rtt;
	}




	
	total_snt =  tp->bytes_acked - pk3c->nums->bytesacked_tot;
	if(total_snt > BIG_VAL_S32)
	        interval->total_snt = 0;
	pk3c->nums->bytesacked_tot = tp->bytes_acked;

	curtime = ktime_u64()/1000;
	pk3c->nums->bytesacked_tot_send_start = curtime+1;

	pk3c->monitor_lost->total_bytes_lost = 0;
	
	init_monitor(sk, pk3c);
	pk3c->monitor_lost->valid = 1;

	
	if(!pk3c->nums->handbrake)
		new_rate = pk3c_decide_rate(pk3c);
	else
		new_rate = pk3c->nums->handbrake;

	new_rate = min(new_rate, MAX_RATE_LIMIT);
	new_rate = max(new_rate, MIN_RATE_LIMIT);

	if (new_rate == MIN_RATE_LIMIT || pk3c->nums->rate == MIN_RATE_LIMIT || new_rate != pk3c->nums->rate || pk3c->nums->handbrake) {
		cprintk(KERN_DEBUG "%d decide: on new rate %d %d (%d)\n",
		       pk3c->nums->id, pk3c->nums->rate < new_rate, new_rate,
		       pk3c->nums->decisions_count);
		pk3c->moving = true;
		pk3c->nums->last_rate = pk3c->nums->rate;
		
		can_enter_fastmoving_based_on_throughput = false;
		if(pk3c->nums->thr && abs(new_rate - pk3c->nums->thr) < pk3c->nums->thr*5 && new_rate*2 < pk3c->nums->thr*3 && new_rate > ((pk3c->nums->maxrate+pk3c->nums->minrate)/2 - (pk3c->nums->maxrate+pk3c->nums->minrate)/4) )
			can_enter_fastmoving_based_on_throughput = true;

		if(new_rate < MIN_RATE_LIMIT*2)
			can_enter_fastmoving_based_on_throughput = true;
		if(calc_T_formula(new_rate, pk3c->lrtt, sk) < MIN_X_RELIABLE_VALUE && can_enter_fastmoving_based_on_throughput) {
			cprintk(KERN_DEBUG "%i start moving fast. The rate/mss * rtt/1000000 = %llu\n", pk3c->nums->id, calc_T_formula(new_rate, pk3c->lrtt, sk));
			pk3c->last_decision = PK3C_RATE_UP;
			pk3c->moving_fast = true;
		}
		pk3c->nums->rate = new_rate;
		pk3c_setup_intervals_moving(pk3c, tp->bytes_acked);
		pk3c->nums->prev_last_rate = pk3c->nums->last_rate;
		pk3c->nums->probing_cnt = 1;
	} else {
		cprintk(KERN_DEBUG "%d decide: stay %lld (%d)\n", pk3c->nums->id,
		       pk3c->nums->rate, pk3c->nums->decisions_count);

		if(pk3c->nums->probing_cnt < 126)
			pk3c->nums->probing_cnt++;
		pk3c_setup_intervals_probing(pk3c);
	}

	pk3c->nums->rate = new_rate;
	start_interval(sk, pk3c);
	pk3c->nums->decisions_count++;
}


static void pk3c_update_step_params(struct pk3c_data *pk3c, s64 step) {
	if ((step > 0) == (pk3c->nums->rate > pk3c->nums->last_rate)) {
		if (pk3c->nums->swing_buffer > 0)
			pk3c->nums->swing_buffer--;
		else
			pk3c->nums->amplifier++;
	} else {
		pk3c->nums->swing_buffer = min(pk3c->nums->swing_buffer + 1, PK3C_MAX_SWING_BUFFER);
		pk3c->nums->amplifier = PK3C_AMP_MIN;
		pk3c->nums->change_bound = PK3C_MIN_CHANGE_BOUND;
	}
}


static s64 pk3c_apply_change_bound(struct pk3c_data *pk3c, s64 step) {
	s32 step_sign;
	s64 change_ratio;
	if (pk3c->nums->rate == 0)
		return step;

	step_sign = step > 0 ? 1 : -1;
	step *= step_sign;
	change_ratio = (PK3C_SCALE * step) / pk3c->nums->rate;

	if (change_ratio > pk3c->nums->change_bound) {
		step = (pk3c->nums->rate * pk3c->nums->change_bound) / PK3C_SCALE;
		pk3c->nums->change_bound += PK3C_CHANGE_BOUND_STEP;
	} else {
		pk3c->nums->change_bound = PK3C_MIN_CHANGE_BOUND;
	}
	return step_sign * step;
}



static u32 pk3c_decide_rate_moving(struct sock *sk, struct pk3c_data *pk3c)
{
	struct pk3c_interval *interval = &pk3c->intervals[0];
	s64 utility, prev_utility;
	s64 grad, step, min_step;
	struct tcp_sock *tp = tcp_sk(sk);
	

	prev_utility = interval->utility;
	if(!pk3c->nums->handbrake) {
	        (*pk3c->util_func)(pk3c, interval, sk);
		
	
		interval->total_snt = tp->bytes_acked - pk3c->nums->bytesacked_tot;
		if(interval->total_snt > BIG_VAL_S32)
			interval->total_snt = 0;
		pk3c->nums->bytesacked_tot = tp->bytes_acked;
		interval->send_start = pk3c->nums->bytesacked_tot_send_start;
		interval->recv_end = ktime_u64()/1000;
	
		pk3c->monitor_lost->total_bytes_lost = 0;
	
		pk3c->nums->bytesacked_tot_send_start = interval->recv_end+1;
	
	        utility = interval->utility;
	} else {
		utility = 0;
	}

	

	init_monitor(sk, pk3c);
	pk3c->monitor_lost->valid = 1;

	
	if(pk3c->nums->handbrake) {
		if(pk3c->nums->handbrake>=MIN_RATE_LIMIT && pk3c->nums->handbrake<=MAX_RATE_LIMIT) {
			cprintk(KERN_DEBUG "%i EMERGENCY-IN-WORK DEBUG HANDBRAKE update rate to %lld, because of previous handbrake\n", pk3c->nums->id, pk3c->nums->handbrake);
			pk3c->nums->rate = pk3c->nums->handbrake;
			pk3c->nums->handbrake = 0;

			return pk3c->nums->rate;
		} else {
			pk3c->nums->rate = pk3c->nums->last_rate;
			pk3c->nums->rate = min(pk3c->nums->rate, MAX_RATE_LIMIT);
			pk3c->nums->rate = max(pk3c->nums->rate, MIN_RATE_LIMIT);
			pk3c->nums->handbrake = 0;
			return pk3c->nums->rate;
		}
	}
	
	cprintk(KERN_DEBUG "%d mv: pr %lld pu %lld nr %llu nu %lld\n",
	       pk3c->nums->id, pk3c->nums->last_rate, prev_utility, pk3c->nums->rate, utility);

	grad = pk3c_calc_util_grad(pk3c->nums->rate, utility, pk3c->nums->last_rate, prev_utility);

	step = grad * PK3C_GRAD_STEP_SIZE;
	pk3c_update_step_params(pk3c, step);
	step *= pk3c->nums->amplifier;
	step /= PK3C_SCALE;
	step = pk3c_apply_change_bound(pk3c, step);

	min_step = (pk3c->nums->rate * PK3C_MIN_RATE_DIFF_RATIO_FOR_GRAD) / PK3C_SCALE;
	min_step *= 11;
	min_step /= 10;
	if (step >= 0 && step < min_step)
		step = min_step;
	else if (step < 0 && step > -1 * min_step)
		step = -1 * min_step;

	cprintk(KERN_DEBUG "%d mv: grad %lld STEP %lld amp %d min_step %lld\n",
	       pk3c->nums->id, grad, step, pk3c->nums->amplifier, min_step);

	if(pk3c->nums->rate + step > MAX_RATE_LIMIT) {
		return MAX_RATE_LIMIT;
	}
	if(pk3c->nums->rate + step < MIN_RATE_LIMIT) {
		return pk3c->nums->rate;
	}

	return pk3c->nums->rate + step;
}


static void pk3ctcp_init(struct sock* sk)
{
	struct pk3c_data *pk3c = inet_csk_ca(sk);
	struct tcp_sock *tp = tcp_sk(sk);

	pk3c->intervals = kzalloc(sizeof(struct pk3c_interval) * PK3C_INTERVALS,
				 GFP_ATOMIC);

	if (!pk3c->intervals) {
		cprintk(KERN_ERR "init fails (cannot allocate intervals)\n");
		return;
	}

	pk3c->nums = kzalloc(sizeof(struct pk3c_nums), GFP_ATOMIC);

	if (!pk3c->nums) {
		kfree(pk3c->intervals);
		pk3c->intervals = NULL;
		cprintk(KERN_ERR "init fails (cannot allocate nums)\n");
		return;
	}

	pk3c->monitor_lost = kzalloc(sizeof(struct pk3c_monitor), GFP_ATOMIC);
	if (!pk3c->monitor_lost) {
		kfree(pk3c->nums);
		pk3c->nums = NULL;
		kfree(pk3c->intervals);
		pk3c->intervals = NULL;
		cprintk(KERN_ERR "init fails (cannot allocate monitor_lost)\n");
		return;
	}


	cprintk(KERN_DEBUG "Called pk3ctcp_init\n");

	init_monitor(sk, pk3c);
	pk3c->monitor_lost->total_bytes_lost = 0;
	
	pk3c->lrtt = 0;
	pk3c->nums->minrtt = BIG_VAL_S32;
	pk3c->nums->minrtt_rate = MIN_RATE_LIMIT;
	pk3c->nums->handbrake = 0;
	pk3c->nums->pcktscnt = 1;
	pk3c->nums->bytesacked_tot = tp->bytes_acked;
	pk3c->nums->lostpckts = 0;
	pk3c->nums->thr = 0;
	pk3c->nums->thr_prev = 0;
	pk3c->nums->thr_prev_cnt = 0;
	pk3c->nums->spare = 0;
	pk3c->nums->id = atomic_inc_return(&id_);


	pk3c->nums->amplifier = PK3C_AMP_MIN;
	pk3c->nums->swing_buffer = 0;
	pk3c->nums->change_bound = PK3C_MIN_CHANGE_BOUND;
	pk3c->nums->pcktscnt_prev = 0;
	pk3c->last_decision = PK3C_RATE_STAY;
	pk3c->nums->probing_cnt = 1;
	pk3c->nums->decisions_count = 0;
	pk3c->nums->lostcnt = 0;

	pk3c->nums->minrate = BIG_VAL_S32;
	pk3c->nums->maxrate = 0;

	pk3c->nums->snd_count = 0;

	pk3c->nums->rate = slow_start_start;
	pk3c->nums->last_rate = 0;
	pk3c->nums->prev_last_rate = 0;

	pk3c->nums->lost_base = 0;
	pk3c->nums->delivered_base = 0;
	pk3c->nums->cnt_no_lost_intervals = PK3C_MIN_INTERVALS_BEFORE_UPDOWN_HANDBRAKE;
	pk3c->nums->slow_start_prev_rtt = 0;

	pk3c->nums->last_rate = PK3C_RATE_MIN*512;
	tcp_sk(sk)->snd_ssthresh = TCP_INFINITE_SSTHRESH;
	pk3c->start_mode = true;
	pk3c->wait = false;
	cprintk(KERN_DEBUG "init, so set pk3c->wait = false\n");
	pk3c->moving = false;
	pk3c->intervals[0].utility = S64_MIN;

	pk3c->util_func = &pk3c_calc_utility_vivace;

	pk3c_setup_intervals_probing(pk3c);
	start_interval(sk,pk3c);

	set_con_cnt(atomic_inc_return(&con_cnt_));

	pk3c->prev_call = LCALL_INIT;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	cmpxchg(&sk->sk_pacing_status, SK_PACING_NONE, SK_PACING_NEEDED);
#endif
}


static void pk3ctcp_release(struct sock *sk)
{

	struct pk3c_data* pk3c = inet_csk_ca(sk);

	if (pk3c->prev_call == LCALL_INIT) {
		pk3c->prev_call = LCALL_FINISH;
		cprintk(KERN_DEBUG "Kfree intervals\n");
		if (pk3c->monitor_lost)
			kfree(pk3c->monitor_lost);
		pk3c->monitor_lost = NULL;
		if (pk3c->intervals)
			kfree(pk3c->intervals);
		pk3c->intervals = NULL;
		if (pk3c->nums)
			kfree(pk3c->nums);
		pk3c->nums = NULL;
		set_con_cnt(atomic_dec_return(&con_cnt_));
	}
}




bool send_interval_ended(struct pk3c_interval *interval, struct tcp_sock *tsk,
			 struct pk3c_data *pk3c)
{
	s64 packets_sent;
	u64 dur;
	u64 compensated_rtt;
	u64 subs_part;
	subs_part = 0;

	packets_sent = pk3c->nums->pcktscnt + tsk->packets_out - interval->packets_sent_base - interval->packets_out_base;


        compensated_rtt = 0;
	if(interval->avg_start_rtt)
		compensated_rtt = interval->avg_start_rtt;
        #if LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)
        subs_part = ((PK3C_SCALE*PK3C_SCALE * tsk->mss_cache*2) / pk3c->nums->rate);
        if(interval->avg_start_rtt > subs_part)
                compensated_rtt = interval->avg_start_rtt - subs_part;
        #endif

	
	dur = (ktime_u64()/1000 - interval->send_start);

	if(dur && compensated_rtt && dur > compensated_rtt && packets_sent > MIN_PACKETS_PER_INTERVAL) {
		if(!interval->recv_start) {
			interval->recv_start = ktime_u64()/1000;
			if(interval->recv_end < interval->recv_start)
				interval->recv_end = interval->recv_start;
			interval->recv_start_acked_bytes = tsk->bytes_acked;
		}
	}


	if(!pk3c->moving_fast && packets_sent < MIN_PACKETS_PER_INTERVAL) {
		return false;
	}


	if(dur > compensated_rtt) {	
		if((!pk3c->moving_fast && pk3c->nums->pcktscnt > interval->packets_sent_base + interval->packets_out_base) ||
		   (pk3c->moving_fast && pk3c->nums->pcktscnt > interval->packets_sent_base + 1)) {

			if(!interval->recv_start) {
				interval->recv_start = ktime_u64()/1000;
				if(interval->recv_end < interval->recv_start)
					interval->recv_end = interval->recv_start;
	
				interval->recv_start_acked_bytes = tsk->bytes_acked;
			}

	  
			interval->packets_ended = pk3c->nums->pcktscnt;
			interval->packets_ended_lost = pk3c->nums->lostpckts; 
			if(tsk->packets_out > interval->packets_out_base)
				interval->packets_ended += tsk->packets_out - interval->packets_out_base;
			interval->packets_out_base = tsk->packets_out;
			return true;
		}
	}
	return false;
}


bool receive_interval_ended(struct pk3c_interval *interval,
			   struct tcp_sock *tsk, struct pk3c_data *pk3c)
{
	if(interval->packets_ended &&
		((!pk3c->moving_fast && interval->packets_ended + 10 + interval->packets_ended_lost + interval->packets_out_base < pk3c->nums->pcktscnt_prev)
		|| (pk3c->moving_fast && interval->packets_ended + 10 + interval->packets_ended_lost + interval->packets_out_base/4 < pk3c->nums->pcktscnt_prev))
	  )	{
		return true;
	}
	return false;
}


static void start_next_send_interval(struct sock *sk, struct pk3c_data *pk3c)
{
	pk3c->send_index++;
	if (pk3c->send_index == PK3C_INTERVALS || pk3c->start_mode || pk3c->moving) {
		pk3c->wait = true;
	}

	start_interval(sk, pk3c);
}


static void
pk3c_update_interval(struct pk3c_interval *interval,	struct pk3c_data *pk3c,
		struct sock *sk)
{
	u64 lost;
	int k;

	lost = 0;

	for(k = 0; k < NUM_OF_PCKTS_FOR_AVG - 1; k++) {
		interval->end_rtt[k] = interval->end_rtt[k+1];
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
	interval->end_rtt[NUM_OF_PCKTS_FOR_AVG-1] = pk3c->lrtt;

#else
	interval->end_rtt[NUM_OF_PCKTS_FOR_AVG-1] = tcp_sk(sk)->srtt_us >> 3;
#endif

	if(interval->min_rtt > interval->end_rtt[NUM_OF_PCKTS_FOR_AVG-1])
		interval->min_rtt = interval->end_rtt[NUM_OF_PCKTS_FOR_AVG-1];
	if(interval->max_rtt < interval->end_rtt[NUM_OF_PCKTS_FOR_AVG-1])
		interval->max_rtt = interval->end_rtt[NUM_OF_PCKTS_FOR_AVG-1];
	

	if (interval->lost + interval->delivered < NUM_OF_PCKTS_FOR_AVG || !interval->cnt_min_pkts) {

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
		if(interval->cnt_min_pkts < NUM_OF_PCKTS_FOR_AVG) {
			interval->start_rtt[interval->cnt_min_pkts] = pk3c->lrtt;
		}
#else
		if(interval->cnt_min_pkts < NUM_OF_PCKTS_FOR_AVG) {
			interval->start_rtt[interval->cnt_min_pkts] = tcp_sk(sk)->srtt_us >> 3;
		}
#endif

		interval->cnt_min_pkts++;

	}

	interval->lost += pk3c->nums->lostpckts - pk3c->nums->lost_base;
	pk3c->nums->lost_base = pk3c->nums->lostpckts;

	interval->delivered += pk3c->nums->pcktscnt - pk3c->nums->delivered_base;
	pk3c->nums->delivered_base = pk3c->nums->pcktscnt;
}


static void pk3c_decide_moving(struct sock *sk, struct pk3c_data *pk3c)
{	
	struct tcp_sock *tp = tcp_sk(sk);
	s64 new_rate = pk3c_decide_rate_moving(sk, pk3c);
	enum PK3C_DECISION decision = pk3c_get_decision(pk3c, new_rate);
	enum PK3C_DECISION last_decision = pk3c->last_decision;
	s64 packet_min_rate = (USEC_PER_SEC * PK3C_RATE_MIN_PACKETS_PER_RTT *
			       tcp_sk(sk)->mss_cache) / pk3c_get_rtt(sk);
	if(packet_min_rate >= MIN_RATE_LIMIT/2 && packet_min_rate < MAX_RATE_LIMIT)
		new_rate = max(new_rate, packet_min_rate);
	new_rate = min(new_rate, MAX_RATE_LIMIT);
	new_rate = max(new_rate, MIN_RATE_LIMIT/2);
	pk3c->nums->prev_last_rate = pk3c->nums->last_rate;
	pk3c->nums->last_rate = pk3c->nums->rate;
	cprintk(KERN_DEBUG "%d moving: new rate %lld (%d) old rate %lld\n",
	       pk3c->nums->id, new_rate,
	       pk3c->nums->decisions_count, pk3c->nums->last_rate);



	pk3c->nums->rate = new_rate;
	if (decision != last_decision || pk3c->nums->handbrake) {


#ifdef USE_PROBING
		pk3c->moving = false;
		pk3c_setup_intervals_probing(pk3c);
#else
		pk3c_setup_intervals_moving(pk3c, tp->bytes_acked);
#endif
		pk3c->nums->handbrake = 0;
	} else {
		pk3c_setup_intervals_moving(pk3c, tp->bytes_acked);
	}

	pk3c->last_decision = decision;
	
	start_interval(sk, pk3c);
}


static void pk3c_decide_fastmoving(struct sock *sk, struct pk3c_data *pk3c)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct pk3c_interval *interval = &pk3c->intervals[0];
	(*pk3c->util_func)(pk3c, interval, sk);

	interval->total_snt = tp->bytes_acked - pk3c->nums->bytesacked_tot;
	pk3c->nums->bytesacked_tot = tp->bytes_acked;
	if(interval->total_snt > BIG_VAL_S32)
		interval->total_snt = 0;

	interval->send_start = pk3c->nums->bytesacked_tot_send_start;
	interval->recv_end = ktime_u64()/1000;
	
	pk3c->monitor_lost->total_bytes_lost = 0;
	pk3c->nums->bytesacked_tot_send_start = interval->recv_end+1;

	
	s64 rate_increase = pk3c->nums->rate/100;

	if(pk3c->lrtt > 100 && pk3c->lrtt < 500000) {
		rate_increase =	tp->mss_cache * (1000000/pk3c->lrtt);
	}
	if(PK3C_MIN_CHANGE_BOUND * pk3c->nums->rate / PK3C_SCALE > rate_increase)
		rate_increase = PK3C_MIN_CHANGE_BOUND * pk3c->nums->rate / PK3C_SCALE;

        s64 new_rate = pk3c->nums->rate + rate_increase;

	pk3c->nums->prev_last_rate = pk3c->nums->last_rate;
	pk3c->nums->last_rate = pk3c->nums->rate;


	init_monitor(sk, pk3c);
	pk3c->monitor_lost->valid = 1;
	

	enum PK3C_DECISION decision = PK3C_RATE_UP;
	enum PK3C_DECISION last_decision = pk3c->last_decision;


	
	if(decision != last_decision || interval->lost > 0 || calc_T_formula(new_rate, pk3c->lrtt, sk) >= THE_T_VALUE) {
		if(decision != last_decision || calc_T_formula(new_rate, pk3c->lrtt, sk) >= THE_T_VALUE || interval->lost > 2 ) {


	cprintk(KERN_DEBUG "%i end moving_fast: pk3c_setup_intervals_probing because of: decision %i != last_decision %i || pk3c->nums->handbrake %i || interval->lost %i || thrpt x 3 %llu >= rate %llu, %lld\n", pk3c->nums->id, decision, last_decision, pk3c->nums->handbrake, interval->lost, pk3c->nums->thr_prev, pk3c->nums->rate, calc_T_formula(new_rate, pk3c->lrtt, sk));

	  
#ifdef USE_PROBING
		pk3c->moving = false;
		pk3c->moving_fast = false;
			cprintk(KERN_DEBUG "starting probing\n");
		pk3c_setup_intervals_probing(pk3c);
#else
		pk3c_setup_intervals_moving(pk3c, tp->bytes_acked);
#endif
		pk3c->nums->handbrake = 0;
	} else {
		if(interval->lost > 0) {
			s64 rate_decrease = pk3c->nums->rate/100;
			if(pk3c->lrtt > 100 && pk3c->lrtt < 500000) {
				rate_decrease =	tp->mss_cache * (1000000/pk3c->lrtt);
			}

			if(pk3c->nums->rate-rate_decrease/8 > ((pk3c->nums->maxrate+pk3c->nums->minrate)/2 - (pk3c->nums->maxrate+pk3c->nums->minrate)/4) )	
				pk3c->nums->rate -= rate_decrease/8;
		} else {
			pk3c->nums->rate = new_rate;
		}
                pk3c_setup_intervals_moving(pk3c, tp->bytes_acked);
        }
	} else {
		pk3c->nums->rate = new_rate;
		pk3c_setup_intervals_moving(pk3c, tp->bytes_acked);
	}

	pk3c->last_decision = decision;
	
	start_interval(sk, pk3c);
}


static void pk3c_decide_slow_start(struct sock *sk, struct pk3c_data *pk3c)
{
	struct pk3c_interval *interval = &pk3c->intervals[0];
	s64 utility, prev_utility;
	struct tcp_sock *tp = tcp_sk(sk);


	prev_utility = interval->utility;
	(*pk3c->util_func)(pk3c, interval, sk);

	interval->total_snt = tp->bytes_acked - pk3c->nums->bytesacked_tot;
	pk3c->nums->bytesacked_tot = tp->bytes_acked;
	pk3c->nums->bytesacked_tot_send_start = ktime_u64()/1000 + 1;
	if(interval->total_snt > BIG_VAL_S32)
		interval->total_snt = 0;
	
	pk3c->monitor_lost->total_bytes_lost = 0;
	
	
	utility = interval->utility;

	

	init_monitor(sk, pk3c);	
	pk3c->monitor_lost->valid = 1;

	
	if (!pk3c->nums->handbrake && utility > prev_utility && pk3c->nums->rate < MAX_RATE_LIMIT) {
		pk3c->nums->last_rate = pk3c->nums->rate;

		pk3c->nums->rate += pk3c->nums->rate / 2;
		interval->utility = utility;
		interval->rate = pk3c->nums->rate;

	        if(pk3c->nums->thr_prev && pk3c->nums->thr_prev > pk3c->nums->rate / 10000 &&
	           pk3c->nums->thr_prev * 5 >= MIN_RATE_LIMIT &&
	           pk3c->nums->thr_prev <= MAX_RATE_LIMIT &&
                   pk3c->nums->thr_prev * 3 / 2 < interval->rate
	          ) {
		        interval->rate = pk3c->nums->thr_prev * 3 / 2;
		        if(pk3c->nums->thr > pk3c->nums->thr_prev)
			    interval->rate = pk3c->nums->thr * 3 / 2;

		        if(interval->rate < MIN_RATE_LIMIT) {
			    interval->rate = MIN_RATE_LIMIT;
		        }
	            }	
		pk3c->nums->rate = interval->rate;

		cprintk(KERN_INFO "%d pk3c_decide_slow_start set interval->rate = %i thr = %lu\n", pk3c->nums->id, interval->rate, pk3c->nums->thr);
		pk3c->send_index = 0;
		interval->send_start = 0;
		interval->send_end = 0;
		pk3c->receive_index = 0;
		pk3c->wait = false;
		cprintk(KERN_DEBUG "decide_slow_start and utility > prev_utility, so set pk3c->wait = false and set interval->rate to %llu\n", interval->rate);
	} else {
		pk3c->nums->last_rate = pk3c->nums->rate;

		pk3c->nums->rate = pk3c->nums->last_rate;

		if(pk3c->nums->thr > 0 && pk3c->nums->thr > pk3c->nums->last_rate/10000) 
			pk3c->nums->rate = pk3c->nums->thr;

		if(pk3c->nums->thr_prev && pk3c->nums->thr_prev > pk3c->nums->last_rate/10000 && pk3c->nums->thr_prev > pk3c->nums->rate) 
			pk3c->nums->rate = pk3c->nums->thr_prev;

		if(pk3c->nums->rate < MIN_RATE_LIMIT) {
			pk3c->nums->rate = MIN_RATE_LIMIT;
		}
		pk3c->nums->cnt_no_lost_intervals = 3;
		pk3c->start_mode = false;
		pk3c->nums->handbrake = 0;


		pk3c->moving = true;
		pk3c->moving_fast = true;
		cprintk(KERN_DEBUG "%i after slow-start finished, start moving_fast mode\n", pk3c->nums->id);
		pk3c->last_decision = PK3C_RATE_UP;
		pk3c_setup_intervals_moving(pk3c, tp->bytes_acked);
    }
    start_interval(sk, pk3c);
}


static void pk3c_process(struct sock *sk)
{
	struct pk3c_data *pk3c = inet_csk_ca(sk);
	struct tcp_sock *tsk = tcp_sk(sk);
	struct pk3c_interval *interval;
	int index;
	u64 before;

	if (pk3c->prev_call != LCALL_INIT && pk3c->prev_call != LCALL_FINISH)
		pk3c->prev_call = LCALL_PROCESS;

	if(pk3c->prev_call != LCALL_INIT) {

		return;
	}

	if (!pk3c_valid(pk3c)) {
		cprintk(KERN_DEBUG "!pk3c_valid in pk3c_process(..)\n");
		return;
	}



	pk3c_set_cwnd(sk);
	if (pk3c->loss_state) {
		goto end;
	}
	if (!pk3c->wait) {
		interval = &pk3c->intervals[pk3c->send_index];
		if (interval->send_start && send_interval_ended(interval, tsk, pk3c)) {
			interval->send_end = ktime_u64()/1000;
			if(!interval->recv_start) {
				interval->recv_start = interval->send_end;
				if(interval->recv_end < interval->recv_start)
					interval->recv_end = interval->recv_start;

				interval->recv_start_acked_bytes = tsk->bytes_acked;
			}

	
			start_next_send_interval(sk, pk3c);
		}
	}




	index = pk3c->receive_index;
	interval = &pk3c->intervals[index];

	before = pk3c->nums->pcktscnt_prev;
	pk3c->nums->pcktscnt_prev = pk3c->nums->pcktscnt + pk3c->nums->lostpckts - pk3c->nums->spare;
	
        if (!pk3c->start_mode && !pk3c->moving_fast && !interval->send_end) {
		goto end;
	}

	if (!interval->packets_sent_base) {
		cprintk(KERN_DEBUG "pk3c_process !interval->packets_sent_base, so goto end\n");
		goto end;
	}


	if (before > 10 + interval->packets_sent_base + interval->packets_out_base || pk3c->start_mode || pk3c->moving_fast) {

		pk3c_update_interval(interval, pk3c, sk);
	}

        if (!interval->send_end) {
		goto end;
	}



	if (receive_interval_ended(interval, tsk, pk3c)) {
		if(!interval->recv_end || interval->recv_end == interval->recv_start) {
			interval->recv_end = ktime_u64()/1000;
			interval->recv_end_acked_bytes = tsk->bytes_acked;
		}

	

		pk3c->receive_index++;

		if (pk3c->start_mode) {
			pk3c_decide_slow_start(sk, pk3c);
		} else if (pk3c->moving && pk3c->moving_fast) {
			pk3c_decide_fastmoving(sk, pk3c);
		} else if (pk3c->moving) {
			pk3c_decide_moving(sk, pk3c);
		} else if (pk3c->receive_index == PK3C_INTERVALS) {
			pk3c_decide(pk3c, sk);
		}
	}

end:

	pk3c->nums->spare = 0;
	pk3c->nums->lost_base = pk3c->nums->lostpckts;
	pk3c->nums->delivered_base = pk3c->nums->pcktscnt;


}


static void pk3ctcp_cong_avoid(struct sock* sk, u32 ack, u32 acked)
{
	return pk3c_process(sk);
}


static void check_if_sent(struct sock *sk, struct pk3c_data *pk3c)
{
	struct tcp_sock *tp = tcp_sk(sk);
	
	if (pk3c->nums->snd_count == tp->segs_out) {
		return;
	}
	pk3c->nums->snd_count = tp->segs_out;

	pk3c->monitor_lost->snd_end_seq = tp->snd_nxt;
}


static inline void do_checks(struct sock *sk, struct pk3c_data *pk3c)
{
	check_if_sent(sk, pk3c);
}


static void update_interval_with_received_acks(struct sock *sk, struct pk3c_data *pk3c)
{


	struct tcp_sock *tp = tcp_sk(sk);
	int i,j;
	u32 end_seq;
	struct tcp_sack_block sack_cache[4];


	struct sk_buff *skb;
	int lost_cnt_maybe = 0;



	spin_lock(&sk->sk_write_queue.lock);
	if(tp->retransmit_skb_hint) {
		skb = tp->retransmit_skb_hint;
	} else {
		skb = tcp_write_queue_head(sk);
	}



	skb_queue_walk(&sk->sk_write_queue, skb) {


		if(after(pk3c->nums->skb_lost_counted, skb)) {
			continue;
		}


		if(before(skb, tp->retransmit_skb_hint)) {
			continue;
		}

		if(after(skb, tcp_write_queue_head(sk))) {
			continue;
		}

	
		__u8 sacked = TCP_SKB_CB(skb)->sacked;

		if (sacked & (TCPCB_SACKED_ACKED|TCPCB_SACKED_RETRANS)) {
			continue;
		}
	
		lost_cnt_maybe++;
		if(lost_cnt_maybe >= 10)
			break;
	}
	spin_unlock(&sk->sk_write_queue.lock);



	if (tp->sacked_out && tp->packets_out) {

		memcpy(sack_cache, tp->recv_sack_cache, sizeof(tp->recv_sack_cache));
		for (i = 0; i < ARRAY_SIZE(tp->recv_sack_cache); i++) {
			for (j = i+1; j < ARRAY_SIZE(tp->recv_sack_cache); j++) {
				if (after(sack_cache[i].start_seq, sack_cache[j].start_seq))
				{
					u32 tmp = sack_cache[i].start_seq;
					sack_cache[i].start_seq = sack_cache[j].start_seq;
					sack_cache[j].start_seq = tmp;
					tmp = sack_cache[i].end_seq;
					sack_cache[i].end_seq = sack_cache[j].end_seq;
					sack_cache[j].end_seq = tmp;
				}
			}
		}
	}

	if (tp->sacked_out && tp->packets_out) {
		for (j = 0; j < ARRAY_SIZE(tp->recv_sack_cache); j++) {
		  
			if (sack_cache[j].start_seq != 0 && sack_cache[j].end_seq != 0) {
				if(!pk3c->nums->last_acked_seq && sack_cache[j].start_seq>0) {
					pk3c->nums->last_acked_seq = sack_cache[j].start_seq-1;
				}


				if (before(pk3c->nums->last_acked_seq, sack_cache[j].end_seq)) {
	
					if (after(sack_cache[j].start_seq,pk3c->nums->last_acked_seq)) {
	
					if (!pk3c->nums->snd_end_seq || before(sack_cache[j].start_seq, pk3c->nums->snd_end_seq)) {
					
						end_seq = sack_cache[j].end_seq;
						if(end_seq > tp->snd_nxt)
							end_seq = tp->snd_nxt;
						if(end_seq < sack_cache[j].start_seq)
							end_seq = sack_cache[j].start_seq;

						s32 lost = end_seq - sack_cache[j].start_seq;
						if(pk3c->nums->last_acked_seq > sack_cache[j].start_seq && end_seq > pk3c->nums->last_acked_seq)
							lost = end_seq - pk3c->nums->last_acked_seq;


						if(lost / tp->mss_cache > lost_cnt_maybe) {
							lost = lost_cnt_maybe * tp->mss_cache;

						}
						pk3c->monitor_lost->bytes_lost += lost;

						pk3c->nums->lostpckts += lost / tp->mss_cache;
						pk3c->nums->skb_lost_counted = skb;
						pk3c->monitor_lost->total_bytes_lost += lost;
	
						if(sack_cache[j].end_seq > pk3c->nums->last_acked_seq)
							pk3c->nums->last_acked_seq = sack_cache[j].end_seq;
					}
					if (sack_cache[j].start_seq > pk3c->nums->last_acked_seq && sack_cache[j].start_seq>0) {
						pk3c->nums->last_acked_seq = sack_cache[j].start_seq;
				}
	
				}
				}

			}
		}
	}




	return;
}



static u32 pk3ctcp_recalc_ssthresh(struct sock* sk)
{
	struct pk3c_data *pk3c = inet_csk_ca(sk);

	if (pk3c->prev_call != LCALL_INIT)
		return TCP_INFINITE_SSTHRESH;

	update_interval_with_received_acks(sk, pk3c);
	do_checks(sk, pk3c);

	return TCP_INFINITE_SSTHRESH;
}


static u32 pk3ctcp_undo_cwnd(struct sock* sk) 
{

	return 0;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
static void pk3c_process_skip(struct sock *sk, const struct rate_sample *rs)
#else
static void pk3c_process_skip(struct sock *sk)
#endif
{
        return;
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)
static void pk3ctcp_acked(struct sock *sk, u32 cnt, s32 rtt_us)
#else
static void pk3ctcp_acked(struct sock *sk, const struct ack_sample *acks)
#endif
{
	s32 rtt;
	u32 rcnt;
	struct pk3c_data *pk3c = inet_csk_ca(sk);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)
	rtt = rtt_us;
	rcnt = cnt;
#else
	rtt = acks->rtt_us;
	rcnt = acks->pkts_acked;
#endif


	if(rtt < 0 || rtt > BIG_VAL_S32)
		return;

	if (pk3c->prev_call == LCALL_INIT) {
		pk3c->nums->pcktscnt += rcnt;

		if(!pk3c->lrtt)
			pk3c->lrtt = rtt;
		pk3c->lrtt = (750 * pk3c->lrtt + rtt * 250) / 1000;

		if(pk3c->nums->minrtt > pk3c->lrtt) {
			pk3c->nums->minrtt = pk3c->lrtt;
			pk3c->nums->minrtt_rate = pk3c->nums->rate*9/10;
		}
		if((pk3c->nums->minrtt*125)/100 > pk3c->lrtt) {
			pk3c->nums->minrtt_rate = pk3c->nums->rate*9/10;
		}
	
		update_interval_with_received_acks(sk, pk3c);
		
	
		do_checks(sk, pk3c);
	}

        return pk3c_process(sk);
}


static void pk3c_set_state(struct sock *sk, u8 new_state)
{
	struct pk3c_data *pk3c = inet_csk_ca(sk);
	s64 spare;

	if (pk3c->prev_call != LCALL_INIT && pk3c->prev_call != LCALL_FINISH)
		pk3c->prev_call = LCALL_STATE;

	if (pk3c->prev_call != LCALL_INIT || (!pk3c_valid(pk3c))) {
		cprintk(KERN_ERR "pk3c_set_state called before init-connection or !pk3c_valid(..) error\n");

		return;
	}


	if (pk3c->loss_state && new_state != TCP_CA_Loss) {
		spare = pk3c->nums->lostpckts - pk3c->nums->lost_base + tcp_packets_in_flight(tcp_sk(sk)) - tcp_sk(sk)->packets_out;

		pk3c->nums->spare += spare;
		cprintk(KERN_DEBUG "%d loss ended: spare %lld, pk3c_setup_intervals_probing\n", pk3c->nums->id, spare);
		pk3c->loss_state = false;
		pk3c_setup_intervals_probing(pk3c);
		start_interval(sk, pk3c);
	}
	else if (!pk3c->loss_state && new_state == TCP_CA_Loss) {
		cprintk(KERN_DEBUG "LOSS MAYBE\n");
	}
}


static u32 pk3c_tso_segs_goal(struct sock *sk)
{
	return 1;
}


static struct tcp_congestion_ops pk3ctcp __read_mostly = {
  .init           = pk3ctcp_init,
  .release        = pk3ctcp_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
  .cong_control   = pk3c_process_skip,
#endif
  .ssthresh       = pk3ctcp_recalc_ssthresh,
  .cong_avoid     = pk3ctcp_cong_avoid,
  .set_state      = pk3c_set_state,
  .undo_cwnd      = pk3ctcp_undo_cwnd,
  .pkts_acked     = pk3ctcp_acked,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
  .tso_segs_goal  = pk3c_tso_segs_goal,
#endif
#endif

  .owner          = THIS_MODULE,
  .name           = "pk3c",
};


static int __init pk3ctcp_register(void) 
{
	int ret = -ENOMEM;

	BUILD_BUG_ON(sizeof(struct pk3c_data) > ICSK_CA_PRIV_SIZE);

	init_waitqueue_head(&tcp_probe_pk3c.wait);
	spin_lock_init(&tcp_probe_pk3c.lock);

	if (bufsize == 0)
		return -EINVAL;

	bufsize = roundup_pow_of_two(bufsize);


	if(ret = ver_create(STR_CONST_MODULE_NAME, STR_CONST_MODULE_VERSION)) {
         	goto err0;
	}

	cprintk(KERN_INFO "[PK3C] Successfully inserted protocol module into kernel, time: %llu\n", ktime_u64());

	return tcp_register_congestion_control(&pk3ctcp);

 err0:
	cprintk(KERN_ERR "Error during PK3C module initialization\n");	

	return ret;
}

static void __exit pk3ctcp_unregister(void) 
{




	tcp_unregister_congestion_control(&pk3ctcp);
	ver_destroy();
	cprintk(KERN_INFO "[PK3C] Successfully unloaded protocol module");
}

module_init(pk3ctcp_register);
module_exit(pk3ctcp_unregister);

MODULE_AUTHOR("Ludwig Munchausen");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(STR_CONST_MODULE_NAME);
MODULE_VERSION(STR_CONST_MODULE_VERSION);
