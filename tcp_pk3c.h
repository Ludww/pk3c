#ifndef __TCP_PK3C_H__
#define __TCP_PK3C_H__

#include <net/tcp.h>

enum PK3C_DECISION {
	PK3C_RATE_UP,
	PK3C_RATE_DOWN,
	PK3C_RATE_STAY,
};


#define MIN_PKTS 10

struct pk3c_interval {
	u64 rate;

	u64 recv_start;
	u64 recv_end;

	u64 recv_start_acked_bytes;
	u64 recv_end_acked_bytes;

	u64 send_start;
	u64 send_end;

	s64 start_rtt[MIN_PKTS];
	s64 end_rtt[MIN_PKTS];

	s64 min_rtt;
	s64 max_rtt;

	s64 avg_start_rtt;
	s64 avg_end_rtt;
	int cnt_min_pkts;

 
	u32 packets_sent_base;
	u32 total_snt; 
	u32 packets_out_base;
	u64 packets_ended;
	u64 packets_ended_lost;

	s64 utility;
	u32 lost;
	u32 delivered;

};


struct pk3c_monitor {

	u8 valid;

	u64 start_time;
	
	u32 snd_start_seq;
	u32 snd_end_seq;
	u32 last_acked_seq;
	struct sk_buff *skb_lost_counted;
	u64 bytes_lost;
	u64 total_bytes_lost;
};

struct pk3c_nums {
	u64 snd_count;
	s64 thr;
	s64 thr_prev;
	s64 thr_prev_cnt;
	s64 handbrake;
	
	int id;
	int decisions_count;

	s64 spare;

	s32 amplifier;
	s32 swing_buffer;
	s32 change_bound;

	s64 last_rate;
	s64 prev_last_rate;
	s64 rate;
	s64 minrtt_rate;

	u64 lost_base;
	u64 delivered_base;

	s64 last_acked_seq;
  
	u64 pcktscnt;
	u64 pcktscnt_prev;
	u32 bytesacked_tot;
	u64 bytesacked_tot_send_start;
	u64 lostpckts;
	u32 minrtt;

	u8 probing_cnt;
	u32 lost;
	u32 lostcnt;
	s64 slow_start_prev_rtt;
	u8 cnt_no_lost_intervals;

	u64 minrate;
	u64 maxrate;

	u32 snd_end_seq;
        struct sk_buff *skb_lost_counted;
};

struct pk3c_data {
	struct pk3c_interval *intervals;
	struct pk3c_nums *nums;
	struct pk3c_monitor *monitor_lost;

	int send_index;
	int receive_index;

	void (*util_func)(struct pk3c_data *, struct pk3c_interval *, struct sock *);

	bool start_mode;
	bool moving;
	bool moving_fast;
	bool loss_state;

	bool wait;
	enum PK3C_DECISION last_decision;

	u32 lrtt;


	u8 prev_call;

};

#endif
