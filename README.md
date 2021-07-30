This is TCP/IP congestion control for video streaming/data servers.
How to improve for free quality of network connections?
How to make download data faster for your servers?
If you don't know what all this about, then read first about Cubic congestion control ( https://en.wikipedia.org/wiki/CUBIC_TCP ) and then about similar Google project BBR that is more in common with this project (than if comparing with Cubic).

Take in mind that this PK3C project has nothing in common with any other versions of PCC project, so the new name PK3C suppose to show that
it is based on original https://github.com/PCCproject/PCC-Kernel project that is under BSD/Gplv2 , so original PCC-Kernel included in subdir PCC-Kernel for history reasons
(this PK3C based on original PCC opensource code (included in ./PCC-Kernel/* );
you may find some other related to PCC projects like some userspace control utility, see https://github.com/PCCproject/PCC-Uspace , and some links to original
papers there in PCC-Uspace that are relevant both for PCC-Kernel and regarding original parts taken from PCC-Kernel for this PK3C too,
but this PK3C project is independent and currently not compatible with this PCC-Uspace utility; generally, see LICENSE and ask questions inside github or privately
to ludwigschapiro@gmail.com , so I'm Ludwig and I'm author of diff from PCC-Kernel to
this PK3C;
found one other similar project that is based on original PCC-Kernel: https://github.com/KaiwenZha/PCC-Vivace
, but as far as I know this PK3C gives better results ).

This one is for TCP/IP, but there are other solutions of congestion problem for other protocols (ex. DCCP protocol that is suppose to be better, than TCP/IP or "UDP";
and with better congestion than any in TCP/IP). However, the DCCP is not yet common (not supported and not known), and TCP/IP is the today most used (apart from UDP without congestion or congestion above raw UDP protocol),
so the projects like this PK3C that are for TCP/IP are still actual today (so I recomend you to try using this PK3C instead of Cubic).


Installation

git clone https://github.com/Ludww/pk3c.git

For Red Hat 8, prerequirements:
sudo dnf install -y kernel-devel
(or for Red Hat 7:
yum install kernel-devel
)

Check your kernel version:
uname -a
, and make sure that sources exists:
ex. soft link from /lib/modules/3.10.0-1127.el7.x86_64/build
to dir /usr/src/kernels/3.10.0-1127.el7.x86_64

For compiling module, just run:

sudo make clean

sudo make

insmod ./tcp_pk3c.ko

For testing if compiled well, this last cmd:
insmod ./tcp_pk3c.ko
should give output (eg. then should see "TCP: pk3c registered" at the end of `dmesg`)

, and as root run this command (after module loaded and for making all
outside connections being use PK3C) :

echo "pk3c" > /proc/sys/net/ipv4/tcp_congestion_control


Enjoy!
Now the speed of your internet and browsing should be much faster :)
This module improves speed of uploading (like speed of sending data from your machine to somewhere), but still it improves "feeling" of fast internet for local workstation too,
because it improves slow-start (means improves the speed of connection-initiation)
and both it gives higher overall speed of data upload from machine where you try it.
However, if you try one big file upload test, it could be both faster or a little bit slower comparing to default "cubic"
(but take in mind that one file upload test is nothing and instead you should try 100 or 1000 files uploading simultaneously and then you would see the difference,
or for comparing with cubic try simple 5 connections together running "/bin/bash ./a.sh" where "cat ./a.sh" and server is remote server, ex. server in Google Cloud:

time scp ./IMG_7700.MOV user@server:~/1 &

time scp ./IMG_7700.MOV user@server:~/2 &

time scp ./IMG_7700.MOV user@server:~/3 &

time scp ./IMG_7700.MOV user@server:~/4 &

time scp ./IMG_7700.MOV user@server:~/5 &

- with PK3C this should work about %20 faster, than if with default Cubic
).


Advantages of PK3C.
Generally, there are similar kernel modules (like see http://web.cs.wpi.edu/~claypool/papers/tcp-sat-pam-21/camera-draft.pdf
for comparing behavior of one connection using Cubic, BBR, Hybla and PCC).
And BBR is similar like PCC or like this PK3C (it is rate based and Cubic is Window based).
The difference of BBR from PK3C (or from PCC) is:
1. BBR optimized for Google servers and non-universal (unlikely that would give better results for your servers until if you use it similar way like Google does).
2. BBR not supported with kernels 3.x (and PK3C developed first for kernels 3.x and then for 4.x and works the same way for both, so it works similar for kernels 4.x or kernels 3.x, and this is the only rate based congestion TCP/IP that is good for kernels 3.x,
so you may use it for Red Hat 7.x or other older Linux systems!).
3. PK3C optimized both for high bandwidth (like provider of video streams for multi customers) and both for low-speeds lose connections (comparing with PCC that works good only for high speed connections). Saying generally, the PK3C optimized for corner cases
(when very low-rate or special strange connection), and PCC doesn't.
4. The PK3C is free (dual BSD/GPL license and based on https://github.com/PCCproject/PCC-Kernel by Nogah Frankel ). And the latest PCC became comercial (non available and not based on original PCC-Kernel anymore). And the BBR (that is other analogue of PK3C) is available with "Apache License 2.0", but only for latest 4.x or 5.x kernels. So the advantage of PK3C that is works good for Kernels 3.x too and enough universal. And both BBR is "less powerfull" comparing with PK3C, and PCC is "less or similar powerfull" comparing with PK3C.
5. For more info about PK3C advantages comparing with PCC , see top comments of the tcp_pk3c_main.c (and the main advantage is that slow-start happens much faster for PK3C, than for PCC, and two years of science were for making this solution,
so it means that PK3C guesses optimal rate of any new connection faster and better, than PCC). However, nobody knows what is inside latest versions of "commercial" PCC
(looks like kernel part not published on github even it is based originally on GPLv2),
so cannot say exactly "who won"
-- so overall reason why PK3C better, than original version of PCC, is that it is based both on theory and practice (like additionally to the utility function taken from PCC
in PK3C there is some logic with max/min estimates and with do fast slow-start). In PCC there are no such improvements, because authors of PCC trying to keep
it based on utility function only (without such guesses how to make it best,
but instead based on ideal experience in ideal room with ideal conditions).
6. Of course, the congestion algorithm need to meet advantage of auto-stabilization: if many connections share bottleneck, no one connection suppose to use all of the bandwidth,
but instead all connections together should split these bottleneck in alsmost similar parts. The PK3C meets of this expectation (at least tested that many simultanious connections
with PK3C does this), and the original version of PCC (from github) didn't meet this expectation. The main reason that for making overall multi-connections system being effective
all connections needs to change in time randomly (trying higher and lower values even the connection is stable) and PCC instead keeping as stable as possible, but PK3C does some small variations even for stable links. So PK3C has enough randomeness for being similar or better, than cubic (and PCC likely not).
7. PK3C has two algorithms together: both Vivace being used and some additional "handbrakes" being used (like if Vivace make incorrect decision, then other simpler "handbrake" algorithm improves this decision), so second algorithm fixes input bugs of main algorirthm. However, tried to keep original ideas and algorithm Vivace (utility func) as is.
8. Original PCC was to conservative (like designed for some ideal conditions), and the new PK3C is kind of opensource hippie.


Testing.
For local testing of multi-connections, there is special tool "mahimahi" (for emulation).
Also this module tested with 10Gb+ networks with multi (tousants) connections.




Installation.
Note that for kernel 3 need to run this configuration (shell script commands) before usage of the PK3C (where $1 is eth interface name):

sudo ethtool -K $1 gso off

sudo ethtool -K $1 tso off

sudo ethtool -K $1 gro off

sudo sysctl -w net.core.default_qdisc=fq

sudo sysctl -w net.core.wmem_max=8194300

sudo sysctl -w net.ipv4.tcp_wmem="4096 8194300 8194300"

sudo sysctl -w net.core.rmem_max=4258291

sudo sysctl -w net.ipv4.tcp_rmem="4096 8194300 8194300"

tc qdisc replace dev $1 root fq pacing maxrate 32gbit

tc qdisc replace dev $1 root fq pacing maxrate 32gbit flow_limit 10000 refill_delay 2 limit 10000 buckets 3023 quantum 10000 initial_quantum 15140


, and for kernel 4 the sysctl with net.core.wmem_max, net.ipv4.tcp_wmem, net.core.rmem_max, net.ipv4.tcp_rmem are enough.






Additional testing (comparing with Cubic):
for server use command
iperf3 -s -p 80
and for client
iperf3 -c server -p 80 -P 50
, means run 50 connections simultenyiously.
For PK3C I see the results:
...
[ 64]   0.00-10.16  sec  2.57 MBytes  2.12 Mbits/sec                  receiver
[ 66]   0.00-10.16  sec  2.65 MBytes  2.19 Mbits/sec                  receiver
[ 68]   0.00-10.16  sec  2.54 MBytes  2.10 Mbits/sec                  receiver
[ 70]   0.00-10.16  sec  2.66 MBytes  2.20 Mbits/sec                  receiver
[ 72]   0.00-10.16  sec  2.77 MBytes  2.29 Mbits/sec                  receiver
[ 74]   0.00-10.16  sec  2.63 MBytes  2.17 Mbits/sec                  receiver
[ 76]   0.00-10.16  sec  2.64 MBytes  2.18 Mbits/sec                  receiver
[ 78]   0.00-10.16  sec  2.54 MBytes  2.10 Mbits/sec                  receiver
[ 80]   0.00-10.16  sec  2.44 MBytes  2.01 Mbits/sec                  receiver
[ 82]   0.00-10.16  sec  2.80 MBytes  2.31 Mbits/sec                  receiver
[ 84]   0.00-10.16  sec  2.98 MBytes  2.46 Mbits/sec                  receiver
[ 86]   0.00-10.16  sec  2.65 MBytes  2.19 Mbits/sec                  receiver
[ 88]   0.00-10.16  sec  2.64 MBytes  2.18 Mbits/sec                  receiver
[ 90]   0.00-10.16  sec  2.73 MBytes  2.26 Mbits/sec                  receiver
[ 92]   0.00-10.16  sec  2.76 MBytes  2.28 Mbits/sec                  receiver
[ 94]   0.00-10.16  sec  2.53 MBytes  2.09 Mbits/sec                  receiver
[ 96]   0.00-10.16  sec  2.56 MBytes  2.12 Mbits/sec                  receiver
[ 98]   0.00-10.16  sec  2.63 MBytes  2.17 Mbits/sec                  receiver
[100]   0.00-10.16  sec  2.70 MBytes  2.23 Mbits/sec                  receiver
[102]   0.00-10.16  sec  2.51 MBytes  2.07 Mbits/sec                  receiver
[104]   0.00-10.16  sec  2.49 MBytes  2.06 Mbits/sec                  receiver
[SUM]   0.00-10.16  sec   129 MBytes   106 Mbits/sec                  receiver


, and for cubic I see the results:
[ 72]   0.00-10.16  sec  2.18 MBytes  1.80 Mbits/sec                  receiver
[ 74]   0.00-10.16  sec  2.81 MBytes  2.32 Mbits/sec                  receiver
[ 76]   0.00-10.16  sec  3.20 MBytes  2.64 Mbits/sec                  receiver
[ 78]   0.00-10.16  sec  2.18 MBytes  1.80 Mbits/sec                  receiver
[ 80]   0.00-10.16  sec  2.95 MBytes  2.43 Mbits/sec                  receiver
[ 82]   0.00-10.16  sec  2.45 MBytes  2.02 Mbits/sec                  receiver
[ 84]   0.00-10.16  sec  2.23 MBytes  1.84 Mbits/sec                  receiver
[ 86]   0.00-10.16  sec  2.47 MBytes  2.03 Mbits/sec                  receiver
[ 88]   0.00-10.16  sec  2.96 MBytes  2.45 Mbits/sec                  receiver
[ 90]   0.00-10.16  sec  3.54 MBytes  2.92 Mbits/sec                  receiver
[ 92]   0.00-10.16  sec  2.44 MBytes  2.01 Mbits/sec                  receiver
[ 94]   0.00-10.16  sec  3.17 MBytes  2.61 Mbits/sec                  receiver
[ 96]   0.00-10.16  sec  1.49 MBytes  1.23 Mbits/sec                  receiver
[ 98]   0.00-10.16  sec  2.57 MBytes  2.12 Mbits/sec                  receiver
[100]   0.00-10.16  sec  2.54 MBytes  2.10 Mbits/sec                  receiver
[102]   0.00-10.16  sec   870 KBytes   702 Kbits/sec                  receiver
[104]   0.00-10.16  sec  2.10 MBytes  1.73 Mbits/sec                  receiver
[SUM]   0.00-10.16  sec   124 MBytes   103 Mbits/sec                  receiver,

-- so the results are similar and PK3C balancing of connections better than cubic.


However, if you try "-t 60" param for client of iperf3, for Cubic it would give you
[ 84]   0.00-30.16  sec  7.70 MBytes  2.14 Mbits/sec                  receiver
[ 86]   0.00-30.16  sec  7.39 MBytes  2.06 Mbits/sec                  receiver
[ 88]   0.00-30.16  sec  8.29 MBytes  2.31 Mbits/sec                  receiver
[ 90]   0.00-30.16  sec  7.00 MBytes  1.95 Mbits/sec                  receiver
[ 92]   0.00-30.16  sec  6.41 MBytes  1.78 Mbits/sec                  receiver
[ 94]   0.00-30.16  sec  6.31 MBytes  1.75 Mbits/sec                  receiver
[ 96]   0.00-30.16  sec  9.07 MBytes  2.52 Mbits/sec                  receiver
[ 98]   0.00-30.16  sec  8.28 MBytes  2.30 Mbits/sec                  receiver
[100]   0.00-30.16  sec  6.41 MBytes  1.78 Mbits/sec                  receiver
[102]   0.00-30.16  sec  7.04 MBytes  1.96 Mbits/sec                  receiver
[104]   0.00-30.16  sec  6.09 MBytes  1.69 Mbits/sec                  receiver
[SUM]   0.00-30.16  sec   383 MBytes   106 Mbits/sec                  receiver


and for PK3C:
[ 70]   0.00-30.16  sec  9.33 MBytes  2.60 Mbits/sec                  receiver
[ 72]   0.00-30.16  sec  5.58 MBytes  1.55 Mbits/sec                  receiver
[ 74]   0.00-30.16  sec  7.01 MBytes  1.95 Mbits/sec                  receiver
[ 76]   0.00-30.16  sec  5.84 MBytes  1.62 Mbits/sec                  receiver
[ 78]   0.00-30.16  sec  8.58 MBytes  2.39 Mbits/sec                  receiver
[ 80]   0.00-30.16  sec  8.69 MBytes  2.42 Mbits/sec                  receiver
[ 82]   0.00-30.16  sec  4.46 MBytes  1.24 Mbits/sec                  receiver
[ 84]   0.00-30.16  sec  9.25 MBytes  2.57 Mbits/sec                  receiver
[ 86]   0.00-30.16  sec  6.73 MBytes  1.87 Mbits/sec                  receiver
[ 88]   0.00-30.16  sec  8.68 MBytes  2.41 Mbits/sec                  receiver
[ 90]   0.00-30.16  sec  4.88 MBytes  1.36 Mbits/sec                  receiver
[ 92]   0.00-30.16  sec  9.30 MBytes  2.59 Mbits/sec                  receiver
[ 94]   0.00-30.16  sec  6.52 MBytes  1.81 Mbits/sec                  receiver
[ 96]   0.00-30.16  sec  6.86 MBytes  1.91 Mbits/sec                  receiver
[ 98]   0.00-30.16  sec  7.38 MBytes  2.05 Mbits/sec                  receiver
[100]   0.00-30.16  sec  6.21 MBytes  1.73 Mbits/sec                  receiver
[102]   0.00-30.16  sec  4.23 MBytes  1.18 Mbits/sec                  receiver
[104]   0.00-30.16  sec  4.40 MBytes  1.22 Mbits/sec                  receiver
[SUM]   0.00-30.16  sec   384 MBytes   107 Mbits/sec                  receiver
-- so overall conclusion that PK3C is the same or better for this 50 connections with 107 MBits/sec case.


It is expected that the PK3C highly wins against cubic for the high-loss-rate connections (since it can detect optimal rate even if level of noise or loss rate is high).
Did some testing for such connections, but for this overview skipping the results.




For kernel3 the results are worse and for PK3C need at least 30 seconds of iperf test for seeing this more or less balanced results (and don't forget to init "tc" and other params before running test with kernel3):
[ 70]   0.00-30.16  sec  3.55 MBytes   987 Kbits/sec                  receiver
[ 72]   0.00-30.16  sec  5.87 MBytes  1.63 Mbits/sec                  receiver
[ 74]   0.00-30.16  sec  10.6 MBytes  2.96 Mbits/sec                  receiver
[ 76]   0.00-30.16  sec  8.27 MBytes  2.30 Mbits/sec                  receiver
[ 78]   0.00-30.16  sec  6.29 MBytes  1.75 Mbits/sec                  receiver
[ 80]   0.00-30.16  sec  8.12 MBytes  2.26 Mbits/sec                  receiver
[ 82]   0.00-30.16  sec  7.07 MBytes  1.97 Mbits/sec                  receiver
[ 84]   0.00-30.16  sec  4.73 MBytes  1.31 Mbits/sec                  receiver
[ 86]   0.00-30.16  sec  8.86 MBytes  2.46 Mbits/sec                  receiver
[ 88]   0.00-30.16  sec  6.96 MBytes  1.93 Mbits/sec                  receiver
[ 90]   0.00-30.16  sec  5.51 MBytes  1.53 Mbits/sec                  receiver
[ 92]   0.00-30.16  sec  5.76 MBytes  1.60 Mbits/sec                  receiver
[ 94]   0.00-30.16  sec  5.25 MBytes  1.46 Mbits/sec                  receiver
[ 96]   0.00-30.16  sec  8.26 MBytes  2.30 Mbits/sec                  receiver
[ 98]   0.00-30.16  sec  7.53 MBytes  2.09 Mbits/sec                  receiver
[100]   0.00-30.16  sec  4.59 MBytes  1.28 Mbits/sec                  receiver
[102]   0.00-30.16  sec  8.75 MBytes  2.43 Mbits/sec                  receiver
[104]   0.00-30.16  sec  6.34 MBytes  1.76 Mbits/sec                  receiver
[SUM]   0.00-30.16  sec   351 MBytes  97.7 Mbits/sec                  receiver


, however for cubic for one minute test for kernel3 the results are similar:
[ 32]   0.00-55.54  sec  11.0 MBytes  1.67 Mbits/sec                  receiver
[ 34]   0.00-55.54  sec  14.0 MBytes  2.12 Mbits/sec                  receiver
[ 36]   0.00-55.54  sec  15.7 MBytes  2.38 Mbits/sec                  receiver
[ 38]   0.00-55.54  sec  13.2 MBytes  1.99 Mbits/sec                  receiver
[ 40]   0.00-55.54  sec  13.0 MBytes  1.96 Mbits/sec                  receiver
[ 42]   0.00-55.54  sec  13.2 MBytes  1.99 Mbits/sec                  receiver
[ 44]   0.00-55.54  sec  17.3 MBytes  2.61 Mbits/sec                  receiver
[ 46]   0.00-55.54  sec  14.9 MBytes  2.24 Mbits/sec                  receiver
[ 48]   0.00-55.54  sec  11.2 MBytes  1.70 Mbits/sec                  receiver
[ 50]   0.00-55.54  sec  19.1 MBytes  2.89 Mbits/sec                  receiver
[ 52]   0.00-55.54  sec  10.2 MBytes  1.54 Mbits/sec                  receiver
[ 54]   0.00-55.54  sec  18.6 MBytes  2.82 Mbits/sec                  receiver
[ 56]   0.00-55.54  sec  15.0 MBytes  2.26 Mbits/sec                  receiver
[ 58]   0.00-55.54  sec  10.8 MBytes  1.63 Mbits/sec                  receiver
[ 60]   0.00-55.54  sec  14.5 MBytes  2.19 Mbits/sec                  receiver
[ 62]   0.00-55.54  sec  11.9 MBytes  1.80 Mbits/sec                  receiver
[ 64]   0.00-55.54  sec  13.3 MBytes  2.01 Mbits/sec                  receiver
[ 66]   0.00-55.54  sec  15.0 MBytes  2.27 Mbits/sec                  receiver
[ 68]   0.00-55.54  sec  13.8 MBytes  2.09 Mbits/sec                  receiver
[ 70]   0.00-55.54  sec  16.6 MBytes  2.51 Mbits/sec                  receiver
[ 72]   0.00-55.54  sec  15.0 MBytes  2.26 Mbits/sec                  receiver
[ 74]   0.00-55.54  sec  12.8 MBytes  1.93 Mbits/sec                  receiver
[ 76]   0.00-55.54  sec  15.0 MBytes  2.27 Mbits/sec                  receiver
[ 78]   0.00-55.54  sec  16.3 MBytes  2.46 Mbits/sec                  receiver
[ 80]   0.00-55.54  sec  11.1 MBytes  1.68 Mbits/sec                  receiver
[ 82]   0.00-55.54  sec  15.0 MBytes  2.27 Mbits/sec                  receiver
[ 84]   0.00-55.54  sec  13.0 MBytes  1.96 Mbits/sec                  receiver
[ 86]   0.00-55.54  sec  13.6 MBytes  2.05 Mbits/sec                  receiver
[ 88]   0.00-55.54  sec  14.9 MBytes  2.26 Mbits/sec                  receiver
[ 90]   0.00-55.54  sec  12.6 MBytes  1.91 Mbits/sec                  receiver
[ 92]   0.00-55.54  sec  14.3 MBytes  2.16 Mbits/sec                  receiver
[ 94]   0.00-55.54  sec  11.0 MBytes  1.66 Mbits/sec                  receiver
[ 96]   0.00-55.54  sec  13.8 MBytes  2.08 Mbits/sec                  receiver
[ 98]   0.00-55.54  sec  15.2 MBytes  2.29 Mbits/sec                  receiver
[100]   0.00-55.54  sec  16.6 MBytes  2.51 Mbits/sec                  receiver
[102]   0.00-55.54  sec  13.0 MBytes  1.97 Mbits/sec                  receiver
[104]   0.00-55.54  sec  17.3 MBytes  2.62 Mbits/sec                  receiver
[SUM]   0.00-55.54  sec   707 MBytes   107 Mbits/sec                  receiver

, if trying the same test for 5 parallel connections, for kernel3 the result is (for PK3C):

[  5]   0.00-30.16  sec  82.7 MBytes  23.0 Mbits/sec                  receiver
[  8]   0.00-30.16  sec  83.7 MBytes  23.3 Mbits/sec                  receiver
[ 10]   0.00-30.16  sec  57.7 MBytes  16.0 Mbits/sec                  receiver
[ 12]   0.00-30.16  sec  77.7 MBytes  21.6 Mbits/sec                  receiver
[ 14]   0.00-30.16  sec  65.3 MBytes  18.2 Mbits/sec                  receiver
[SUM]   0.00-30.16  sec   367 MBytes   102 Mbits/sec                  receiver
and
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec   111 MBytes  15.5 Mbits/sec                  receiver
[  8]   0.00-60.16  sec   154 MBytes  21.5 Mbits/sec                  receiver
[ 10]   0.00-60.16  sec   147 MBytes  20.5 Mbits/sec                  receiver
[ 12]   0.00-60.16  sec   189 MBytes  26.3 Mbits/sec                  receiver
[ 14]   0.00-60.16  sec   150 MBytes  21.0 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   752 MBytes   105 Mbits/sec                  receiver
and
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-55.55  sec   140 MBytes  21.2 Mbits/sec                  receiver
[  8]   0.00-55.55  sec   151 MBytes  22.8 Mbits/sec                  receiver
[ 10]   0.00-55.55  sec   135 MBytes  20.5 Mbits/sec                  receiver
[ 12]   0.00-55.55  sec   121 MBytes  18.3 Mbits/sec                  receiver
[ 14]   0.00-55.55  sec   151 MBytes  22.8 Mbits/sec                  receiver
[SUM]   0.00-55.55  sec   700 MBytes   106 Mbits/sec                  receiver
and 10 connections:
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec  62.8 MBytes  8.76 Mbits/sec                  receiver
[  8]   0.00-60.16  sec  40.7 MBytes  5.67 Mbits/sec                  receiver
[ 10]   0.00-60.16  sec  93.7 MBytes  13.1 Mbits/sec                  receiver
[ 12]   0.00-60.16  sec  96.6 MBytes  13.5 Mbits/sec                  receiver
[ 14]   0.00-60.16  sec  72.8 MBytes  10.2 Mbits/sec                  receiver
[ 16]   0.00-60.16  sec  62.5 MBytes  8.71 Mbits/sec                  receiver
[ 18]   0.00-60.16  sec  83.2 MBytes  11.6 Mbits/sec                  receiver
[ 20]   0.00-60.16  sec  98.6 MBytes  13.8 Mbits/sec                  receiver
[ 22]   0.00-60.16  sec  56.8 MBytes  7.92 Mbits/sec                  receiver
[ 24]   0.00-60.16  sec  74.4 MBytes  10.4 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   742 MBytes   103 Mbits/sec                  receiver
and
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-30.16  sec  55.5 MBytes  15.4 Mbits/sec                  receiver
[  8]   0.00-30.16  sec  29.4 MBytes  8.17 Mbits/sec                  receiver
[ 10]   0.00-30.16  sec  32.2 MBytes  8.97 Mbits/sec                  receiver
[ 12]   0.00-30.16  sec  30.7 MBytes  8.53 Mbits/sec                  receiver
[ 14]   0.00-30.16  sec  38.8 MBytes  10.8 Mbits/sec                  receiver
[ 16]   0.00-30.16  sec  46.3 MBytes  12.9 Mbits/sec                  receiver
[ 18]   0.00-30.16  sec  40.0 MBytes  11.1 Mbits/sec                  receiver
[ 20]   0.00-30.16  sec  40.4 MBytes  11.2 Mbits/sec                  receiver
[ 22]   0.00-30.16  sec  29.0 MBytes  8.06 Mbits/sec                  receiver
[ 24]   0.00-30.16  sec  33.5 MBytes  9.32 Mbits/sec                  receiver
[SUM]   0.00-30.16  sec   376 MBytes   105 Mbits/sec                  receiver



, and for kernel3 for cubic similar 5 connections results are (note that need to revert back all networking params before trying cubic, like no *mem* tcp/ip and no tc):
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec   148 MBytes  20.6 Mbits/sec                  receiver
[  8]   0.00-60.16  sec   157 MBytes  21.9 Mbits/sec                  receiver
[ 10]   0.00-60.16  sec   196 MBytes  27.3 Mbits/sec                  receiver
[ 12]   0.00-60.16  sec   106 MBytes  14.8 Mbits/sec                  receiver
[ 14]   0.00-60.16  sec   134 MBytes  18.7 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   741 MBytes   103 Mbits/sec                  receiver
and for 10 connections:
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.17  sec  45.9 MBytes  6.40 Mbits/sec                  receiver
[  8]   0.00-60.17  sec  80.9 MBytes  11.3 Mbits/sec                  receiver
[ 10]   0.00-60.17  sec  99.1 MBytes  13.8 Mbits/sec                  receiver
[ 12]   0.00-60.17  sec  50.1 MBytes  6.99 Mbits/sec                  receiver
[ 14]   0.00-60.17  sec  56.6 MBytes  7.89 Mbits/sec                  receiver
[ 16]   0.00-60.17  sec  54.9 MBytes  7.65 Mbits/sec                  receiver
[ 18]   0.00-60.17  sec  76.6 MBytes  10.7 Mbits/sec                  receiver
[ 20]   0.00-60.17  sec  59.4 MBytes  8.29 Mbits/sec                  receiver
[ 22]   0.00-60.17  sec  97.1 MBytes  13.5 Mbits/sec                  receiver
[ 24]   0.00-60.17  sec  62.4 MBytes  8.71 Mbits/sec                  receiver
[SUM]   0.00-60.17  sec   683 MBytes  95.2 Mbits/sec                  receiver
--- so you can see that PK3C wins with 106 instead of 103 for 5 connections and with 103 instead of 95.2 for 10 connections.


For kernel4 for similar 5 connections results are, for PK3C:
[  5]  59.00-60.00  sec  2.50 MBytes  21.0 Mbits/sec   14   1.17 MBytes       
[  7]  59.00-60.00  sec  2.25 MBytes  18.8 Mbits/sec   11    866 KBytes       
[  9]  59.00-60.00  sec  2.27 MBytes  19.1 Mbits/sec    4    542 KBytes       
[ 11]  59.00-60.00  sec  2.48 MBytes  20.8 Mbits/sec    7    891 KBytes       
[ 13]  59.00-60.00  sec  3.74 MBytes  31.4 Mbits/sec    9   1.01 MBytes       
[SUM]  59.00-60.00  sec  13.2 MBytes   111 Mbits/sec   45 
     or (similar 5 connections, but if from sender side of view and with Retr):
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec   142 MBytes  19.9 Mbits/sec  6178             sender
[  5]   0.00-60.16  sec   138 MBytes  19.2 Mbits/sec                  receiver
[  7]   0.00-60.00  sec   124 MBytes  17.3 Mbits/sec  5250             sender
[  7]   0.00-60.16  sec   118 MBytes  16.5 Mbits/sec                  receiver
[  9]   0.00-60.00  sec   125 MBytes  17.5 Mbits/sec  5557             sender
[  9]   0.00-60.16  sec   120 MBytes  16.8 Mbits/sec                  receiver
[ 11]   0.00-60.00  sec   110 MBytes  15.4 Mbits/sec  4120             sender
[ 11]   0.00-60.16  sec   104 MBytes  14.6 Mbits/sec                  receiver
[ 13]   0.00-60.00  sec   179 MBytes  25.0 Mbits/sec  9207             sender
[ 13]   0.00-60.16  sec   174 MBytes  24.3 Mbits/sec                  receiver
[SUM]   0.00-60.00  sec   680 MBytes  95.1 Mbits/sec  30312             sender
[SUM]   0.00-60.16  sec   655 MBytes  91.3 Mbits/sec                  receiver
, and 10 connections:
[  5]  60.00-60.16  sec   116 KBytes  5.86 Mbits/sec                  
[  8]  60.00-60.16  sec   223 KBytes  11.3 Mbits/sec                  
[ 10]  60.00-60.16  sec   351 KBytes  17.8 Mbits/sec                  
[ 12]  60.00-60.16  sec   135 KBytes  6.84 Mbits/sec                  
[ 14]  60.00-60.16  sec   358 KBytes  18.1 Mbits/sec                  
[ 16]  60.00-60.16  sec   381 KBytes  19.3 Mbits/sec                  
[ 18]  60.00-60.16  sec   323 KBytes  16.4 Mbits/sec                  
[ 20]  60.00-60.16  sec   172 KBytes  8.72 Mbits/sec                  
[ 22]  60.00-60.16  sec   257 KBytes  13.0 Mbits/sec                  
[ 24]  60.00-60.16  sec   224 KBytes  11.4 Mbits/sec                  
[SUM]  60.00-60.16  sec  2.48 MBytes   129 Mbits/sec ,
and if similar 10 connections with Retr (from sender point of view and this time 105 MBits/sec of sender speed for similar 129 MBits/sec of receiver, means that
iperf3 can make diff results from sender point of view and from receiver point of view, but anyway this is higher than similar cubic ~90MBits/sec result):
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec  48.8 MBytes  6.82 Mbits/sec  748             sender
[  5]   0.00-60.16  sec  43.2 MBytes  6.03 Mbits/sec                  receiver
[  7]   0.00-60.00  sec  62.5 MBytes  8.74 Mbits/sec  972             sender
[  7]   0.00-60.16  sec  57.8 MBytes  8.06 Mbits/sec                  receiver
[  9]   0.00-60.00  sec  87.5 MBytes  12.2 Mbits/sec  1114             sender
[  9]   0.00-60.16  sec  82.3 MBytes  11.5 Mbits/sec                  receiver
[ 11]   0.00-60.00  sec  68.8 MBytes  9.61 Mbits/sec  1174             sender
[ 11]   0.00-60.16  sec  63.0 MBytes  8.78 Mbits/sec                  receiver
[ 13]   0.00-60.00  sec  87.5 MBytes  12.2 Mbits/sec  1315             sender
[ 13]   0.00-60.16  sec  83.0 MBytes  11.6 Mbits/sec                  receiver
[ 15]   0.00-60.00  sec  62.5 MBytes  8.74 Mbits/sec  1073             sender
[ 15]   0.00-60.16  sec  57.8 MBytes  8.06 Mbits/sec                  receiver
[ 17]   0.00-60.00  sec  90.0 MBytes  12.6 Mbits/sec  1346             sender
[ 17]   0.00-60.16  sec  85.6 MBytes  11.9 Mbits/sec                  receiver
[ 19]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  1263             sender
[ 19]   0.00-60.16  sec  67.2 MBytes  9.36 Mbits/sec                  receiver
[ 21]   0.00-60.00  sec  80.0 MBytes  11.2 Mbits/sec  1268             sender
[ 21]   0.00-60.16  sec  74.3 MBytes  10.4 Mbits/sec                  receiver
[ 23]   0.00-60.00  sec  87.5 MBytes  12.2 Mbits/sec  1455             sender
[ 23]   0.00-60.16  sec  82.5 MBytes  11.5 Mbits/sec                  receiver
[SUM]   0.00-60.00  sec   748 MBytes   105 Mbits/sec  11728             sender
[SUM]   0.00-60.16  sec   697 MBytes  97.1 Mbits/sec                  receiver
-- and you can see that for kernel4 similar algorithm works more accurate giving higher results (since the rtt/etc measurements in kernel3 much more noisy than in kernel4).


and for kernel4 for cubic:
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec   137 MBytes  19.1 Mbits/sec                  receiver
[  8]   0.00-60.16  sec   144 MBytes  20.0 Mbits/sec                  receiver
[ 10]   0.00-60.16  sec   108 MBytes  15.1 Mbits/sec                  receiver
[ 12]   0.00-60.16  sec   114 MBytes  15.9 Mbits/sec                  receiver
[ 14]   0.00-60.16  sec   109 MBytes  15.2 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   612 MBytes  85.3 Mbits/sec                  receiver
or 10 connections for cubic:
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec  59.3 MBytes  8.27 Mbits/sec                  receiver
[  8]   0.00-60.16  sec  61.7 MBytes  8.60 Mbits/sec                  receiver
[ 10]   0.00-60.16  sec  53.3 MBytes  7.43 Mbits/sec                  receiver
[ 12]   0.00-60.16  sec  52.1 MBytes  7.26 Mbits/sec                  receiver
[ 14]   0.00-60.16  sec  66.0 MBytes  9.20 Mbits/sec                  receiver
[ 16]   0.00-60.16  sec  57.7 MBytes  8.05 Mbits/sec                  receiver
[ 18]   0.00-60.16  sec  67.7 MBytes  9.44 Mbits/sec                  receiver
[ 20]   0.00-60.16  sec  65.6 MBytes  9.15 Mbits/sec                  receiver
[ 22]   0.00-60.16  sec  81.4 MBytes  11.3 Mbits/sec                  receiver
[ 24]   0.00-60.16  sec  61.4 MBytes  8.57 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   626 MBytes  87.3 Mbits/sec                  receiver
or same 10 connections for cubic (kernel4) with Retr:
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec  65.0 MBytes  9.09 Mbits/sec  1013             sender
[  5]   0.00-60.16  sec  59.3 MBytes  8.27 Mbits/sec                  receiver
[  7]   0.00-60.00  sec  67.5 MBytes  9.44 Mbits/sec  1014             sender
[  7]   0.00-60.16  sec  61.7 MBytes  8.60 Mbits/sec                  receiver
[  9]   0.00-60.00  sec  58.8 MBytes  8.21 Mbits/sec  595             sender
[  9]   0.00-60.16  sec  53.3 MBytes  7.43 Mbits/sec                  receiver
[ 11]   0.00-60.00  sec  57.5 MBytes  8.04 Mbits/sec  944             sender
[ 11]   0.00-60.16  sec  52.1 MBytes  7.26 Mbits/sec                  receiver
[ 13]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  747             sender
[ 13]   0.00-60.16  sec  66.0 MBytes  9.20 Mbits/sec                  receiver
[ 15]   0.00-60.00  sec  62.5 MBytes  8.74 Mbits/sec  234             sender
[ 15]   0.00-60.16  sec  57.7 MBytes  8.05 Mbits/sec                  receiver
[ 17]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  1240             sender
[ 17]   0.00-60.16  sec  67.7 MBytes  9.44 Mbits/sec                  receiver
[ 19]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  792             sender
[ 19]   0.00-60.16  sec  65.6 MBytes  9.15 Mbits/sec                  receiver
[ 21]   0.00-60.00  sec  86.2 MBytes  12.1 Mbits/sec  1040             sender
[ 21]   0.00-60.16  sec  81.4 MBytes  11.3 Mbits/sec                  receiver
[ 23]   0.00-60.00  sec  66.2 MBytes  9.26 Mbits/sec  699             sender
[ 23]   0.00-60.16  sec  61.4 MBytes  8.57 Mbits/sec                  receiver
[SUM]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec  8318             sender
[SUM]   0.00-60.16  sec   626 MBytes  87.3 Mbits/sec                  receiver


and for kernel4 for PK3C for 2 connections (one min test):
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec   288 MBytes  40.2 Mbits/sec                  receiver
[  8]   0.00-60.16  sec   304 MBytes  42.4 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   593 MBytes  82.6 Mbits/sec                  receiver

and for kernel3 for cubic for 2 connections (one min test):
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec   306 MBytes  42.7 Mbits/sec                  receiver
[  8]   0.00-60.16  sec   310 MBytes  43.3 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   616 MBytes  85.9 Mbits/sec                  receiver

, and for kernel3 for PK3C for 2 connections (one min test):
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec   341 MBytes  47.6 Mbits/sec                  receiver
[  8]   0.00-60.16  sec   388 MBytes  54.1 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   729 MBytes   102 Mbits/sec                  receiver

, and for kernel4 for cubic for 2 connections (one min test):
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.16  sec   311 MBytes  43.4 Mbits/sec                  receiver
[  8]   0.00-60.16  sec   280 MBytes  39.0 Mbits/sec                  receiver
[SUM]   0.00-60.16  sec   591 MBytes  82.4 Mbits/sec                  receiver
-- so for small amount of connections cubic does much worse.


Before making conslusion, I should give at least few other small examples for higher speed networks:

kernel4 local 10 connections with cubic:
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[  8]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 10]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 12]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 14]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 16]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 18]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 20]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 22]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 24]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[SUM]   0.00-60.03  sec  6.58 GBytes   941 Mbits/sec                  receiver
and similar with cubic from sender point of view for seeing Retr:
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[  5]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[  7]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[  7]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[  9]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[  9]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 11]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[ 11]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 13]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[ 13]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 15]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[ 15]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 17]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[ 17]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 19]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[ 19]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 21]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[ 21]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[ 23]   0.00-60.00  sec   679 MBytes  94.9 Mbits/sec    0             sender
[ 23]   0.00-60.03  sec   673 MBytes  94.1 Mbits/sec                  receiver
[SUM]   0.00-60.00  sec  6.63 GBytes   949 Mbits/sec    0             sender
[SUM]   0.00-60.03  sec  6.58 GBytes   941 Mbits/sec                  receiver


, similar kernel4 local 10 connections with PK3C:
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-60.03  sec   676 MBytes  94.5 Mbits/sec                  receiver
[  8]   0.00-60.03  sec   676 MBytes  94.4 Mbits/sec                  receiver
[ 10]   0.00-60.03  sec   675 MBytes  94.4 Mbits/sec                  receiver
[ 12]   0.00-60.03  sec   677 MBytes  94.6 Mbits/sec                  receiver
[ 14]   0.00-60.03  sec   665 MBytes  92.9 Mbits/sec                  receiver
[ 16]   0.00-60.03  sec   670 MBytes  93.6 Mbits/sec                  receiver
[ 18]   0.00-60.03  sec   663 MBytes  92.7 Mbits/sec                  receiver
[ 20]   0.00-60.03  sec   660 MBytes  92.3 Mbits/sec                  receiver
[ 22]   0.00-60.03  sec   670 MBytes  93.6 Mbits/sec                  receiver
[ 24]   0.00-60.03  sec   676 MBytes  94.4 Mbits/sec                  receiver
[SUM]   0.00-60.03  sec  6.55 GBytes   937 Mbits/sec                  receiver
and similar from sender side (for seeing Retr):
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec   681 MBytes  95.2 Mbits/sec  3069             sender
[  5]   0.00-60.03  sec   676 MBytes  94.5 Mbits/sec                  receiver
[  7]   0.00-60.00  sec   681 MBytes  95.2 Mbits/sec  649             sender
[  7]   0.00-60.03  sec   676 MBytes  94.4 Mbits/sec                  receiver
[  9]   0.00-60.00  sec   681 MBytes  95.2 Mbits/sec  231             sender
[  9]   0.00-60.03  sec   675 MBytes  94.4 Mbits/sec                  receiver
[ 11]   0.00-60.00  sec   682 MBytes  95.4 Mbits/sec  1155             sender
[ 11]   0.00-60.03  sec   677 MBytes  94.6 Mbits/sec                  receiver
[ 13]   0.00-60.00  sec   670 MBytes  93.7 Mbits/sec  1155             sender
[ 13]   0.00-60.03  sec   665 MBytes  92.9 Mbits/sec                  receiver
[ 15]   0.00-60.00  sec   675 MBytes  94.4 Mbits/sec  176             sender
[ 15]   0.00-60.03  sec   670 MBytes  93.6 Mbits/sec                  receiver
[ 17]   0.00-60.00  sec   669 MBytes  93.5 Mbits/sec  825             sender
[ 17]   0.00-60.03  sec   663 MBytes  92.7 Mbits/sec                  receiver
[ 19]   0.00-60.00  sec   666 MBytes  93.1 Mbits/sec  363             sender
[ 19]   0.00-60.03  sec   660 MBytes  92.3 Mbits/sec                  receiver
[ 21]   0.00-60.00  sec   675 MBytes  94.4 Mbits/sec  253             sender
[ 21]   0.00-60.03  sec   670 MBytes  93.6 Mbits/sec                  receiver
[ 23]   0.00-60.00  sec   681 MBytes  95.2 Mbits/sec  1969             sender
[ 23]   0.00-60.03  sec   676 MBytes  94.4 Mbits/sec                  receiver
[SUM]   0.00-60.00  sec  6.60 GBytes   945 Mbits/sec  9845             sender
[SUM]   0.00-60.03  sec  6.55 GBytes   937 Mbits/sec                  receiver
-- you can see that PK3C more aggressive (trying sending a little bit faster than it should be, so Retr happened).



And 100 local connections with cubic (using kernel4):
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    3             sender
[  5]   0.00-60.00  sec  69.5 MBytes  9.71 Mbits/sec                  receiver
[  7]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    0             sender
[  7]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[  9]   0.00-60.00  sec  40.0 MBytes  5.59 Mbits/sec  954             sender
[  9]   0.00-60.00  sec  34.7 MBytes  4.86 Mbits/sec                  receiver
[ 11]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    0             sender
[ 11]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 13]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[ 13]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 15]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    0             sender
[ 15]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 17]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    9             sender
[ 17]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 19]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   12             sender
[ 19]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 21]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    3             sender
[ 21]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 23]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 23]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 25]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 25]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 27]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[ 27]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 29]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 29]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 31]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 31]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 33]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 33]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 35]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 35]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 37]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 37]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 39]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    4             sender
[ 39]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 41]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 41]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 43]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 43]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 45]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 45]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 47]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 47]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 49]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[ 49]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 51]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   10             sender
[ 51]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 53]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 53]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 55]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 55]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 57]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   10             sender
[ 57]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 59]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 59]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 61]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   11             sender
[ 61]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 63]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   11             sender
[ 63]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 65]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[ 65]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 67]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 67]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 69]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 69]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 71]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 71]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 73]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 73]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 75]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 75]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 77]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    3             sender
[ 77]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 79]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[ 79]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 81]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 81]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 83]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 83]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 85]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[ 85]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 87]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[ 87]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 89]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    8             sender
[ 89]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 91]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    9             sender
[ 91]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 93]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   10             sender
[ 93]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 95]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[ 95]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[ 97]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   11             sender
[ 97]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[ 99]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    4             sender
[ 99]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[101]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[101]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[103]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[103]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[105]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[105]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[107]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[107]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[109]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[109]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[111]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    9             sender
[111]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[113]   0.00-60.00  sec  40.0 MBytes  5.59 Mbits/sec  882             sender
[113]   0.00-60.00  sec  35.0 MBytes  4.89 Mbits/sec                  receiver
[115]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[115]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[117]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    3             sender
[117]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[119]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    8             sender
[119]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[121]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[121]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[123]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   11             sender
[123]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[125]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[125]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[127]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[127]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[129]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    8             sender
[129]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[131]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[131]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[133]   0.00-60.00  sec  40.0 MBytes  5.59 Mbits/sec  881             sender
[133]   0.00-60.00  sec  35.0 MBytes  4.89 Mbits/sec                  receiver
[135]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[135]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[137]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   10             sender
[137]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[139]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[139]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[141]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    9             sender
[141]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[143]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[143]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[145]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    9             sender
[145]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[147]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   10             sender
[147]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[149]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[149]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[151]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[151]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[153]   0.00-60.00  sec  40.0 MBytes  5.59 Mbits/sec  939             sender
[153]   0.00-60.00  sec  34.4 MBytes  4.81 Mbits/sec                  receiver
[155]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    9             sender
[155]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[157]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[157]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[159]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[159]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[161]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   10             sender
[161]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[163]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[163]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[165]   0.00-60.00  sec  40.0 MBytes  5.59 Mbits/sec  967             sender
[165]   0.00-60.00  sec  34.4 MBytes  4.82 Mbits/sec                  receiver
[167]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    4             sender
[167]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[169]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[169]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[171]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[171]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[173]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    2             sender
[173]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[175]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    9             sender
[175]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[177]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    2             sender
[177]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[179]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    6             sender
[179]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[181]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    5             sender
[181]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[183]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    0             sender
[183]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[185]   0.00-60.00  sec  40.0 MBytes  5.59 Mbits/sec  948             sender
[185]   0.00-60.00  sec  34.7 MBytes  4.85 Mbits/sec                  receiver
[187]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    0             sender
[187]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[189]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    2             sender
[189]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[191]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    0             sender
[191]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[193]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    9             sender
[193]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[195]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    2             sender
[195]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[197]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    0             sender
[197]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[199]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    8             sender
[199]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[201]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec    7             sender
[201]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[203]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec   11             sender
[203]   0.00-60.00  sec  69.4 MBytes  9.70 Mbits/sec                  receiver
[SUM]   0.00-60.00  sec  7.12 GBytes  1.02 Gbits/sec  6142             sender
[SUM]   0.00-60.00  sec  6.58 GBytes   941 Mbits/sec                  receiver



and 100 similar local connections with PK3C (using kernel40:
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec  77.5 MBytes  10.8 Mbits/sec  527             sender
[  5]   0.00-60.00  sec  72.2 MBytes  10.1 Mbits/sec                  receiver
[  7]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  296             sender
[  7]   0.00-60.00  sec  67.3 MBytes  9.41 Mbits/sec                  receiver
[  9]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  258             sender
[  9]   0.00-60.00  sec  66.7 MBytes  9.32 Mbits/sec                  receiver
[ 11]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  256             sender
[ 11]   0.00-60.00  sec  67.6 MBytes  9.45 Mbits/sec                  receiver
[ 13]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  256             sender
[ 13]   0.00-60.00  sec  66.4 MBytes  9.29 Mbits/sec                  receiver
[ 15]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  416             sender
[ 15]   0.00-60.00  sec  71.4 MBytes  9.98 Mbits/sec                  receiver
[ 17]   0.00-60.00  sec  33.8 MBytes  4.72 Mbits/sec  122             sender
[ 17]   0.00-60.00  sec  28.0 MBytes  3.92 Mbits/sec                  receiver
[ 19]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  300             sender
[ 19]   0.00-60.00  sec  69.3 MBytes  9.69 Mbits/sec                  receiver
[ 21]   0.00-60.00  sec  77.5 MBytes  10.8 Mbits/sec  437             sender
[ 21]   0.00-60.00  sec  71.8 MBytes  10.0 Mbits/sec                  receiver
[ 23]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  353             sender
[ 23]   0.00-60.00  sec  66.7 MBytes  9.32 Mbits/sec                  receiver
[ 25]   0.00-60.00  sec  70.0 MBytes  9.79 Mbits/sec  126             sender
[ 25]   0.00-60.00  sec  64.7 MBytes  9.05 Mbits/sec                  receiver
[ 27]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  316             sender
[ 27]   0.00-60.00  sec  68.0 MBytes  9.50 Mbits/sec                  receiver
[ 29]   0.00-60.00  sec  77.5 MBytes  10.8 Mbits/sec  582             sender
[ 29]   0.00-60.00  sec  72.6 MBytes  10.2 Mbits/sec                  receiver
[ 31]   0.00-60.00  sec  80.0 MBytes  11.2 Mbits/sec  804             sender
[ 31]   0.00-60.00  sec  74.1 MBytes  10.4 Mbits/sec                  receiver
[ 33]   0.00-60.00  sec  87.5 MBytes  12.2 Mbits/sec  1132             sender
[ 33]   0.00-60.00  sec  81.7 MBytes  11.4 Mbits/sec                  receiver
[ 35]   0.00-60.00  sec  70.0 MBytes  9.79 Mbits/sec  272             sender
[ 35]   0.00-60.00  sec  65.1 MBytes  9.10 Mbits/sec                  receiver
[ 37]   0.00-60.00  sec  77.5 MBytes  10.8 Mbits/sec  435             sender
[ 37]   0.00-60.00  sec  71.6 MBytes  10.0 Mbits/sec                  receiver
[ 39]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  357             sender
[ 39]   0.00-60.00  sec  69.3 MBytes  9.69 Mbits/sec                  receiver
[ 41]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  579             sender
[ 41]   0.00-60.00  sec  70.1 MBytes  9.80 Mbits/sec                  receiver
[ 43]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  183             sender
[ 43]   0.00-60.00  sec  65.5 MBytes  9.16 Mbits/sec                  receiver
[ 45]   0.00-60.00  sec  77.5 MBytes  10.8 Mbits/sec  587             sender
[ 45]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec                  receiver
[ 47]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  556             sender
[ 47]   0.00-60.00  sec  68.2 MBytes  9.53 Mbits/sec                  receiver
[ 49]   0.00-60.00  sec  45.0 MBytes  6.29 Mbits/sec  122             sender
[ 49]   0.00-60.00  sec  40.2 MBytes  5.62 Mbits/sec                  receiver
[ 51]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  458             sender
[ 51]   0.00-60.00  sec  71.0 MBytes  9.92 Mbits/sec                  receiver
[ 53]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  380             sender
[ 53]   0.00-60.00  sec  69.6 MBytes  9.73 Mbits/sec                  receiver
[ 55]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  216             sender
[ 55]   0.00-60.00  sec  67.4 MBytes  9.42 Mbits/sec                  receiver
[ 57]   0.00-60.00  sec  70.0 MBytes  9.79 Mbits/sec  141             sender
[ 57]   0.00-60.00  sec  64.6 MBytes  9.03 Mbits/sec                  receiver
[ 59]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  260             sender
[ 59]   0.00-60.00  sec  67.2 MBytes  9.39 Mbits/sec                  receiver
[ 61]   0.00-60.00  sec  68.8 MBytes  9.61 Mbits/sec  160             sender
[ 61]   0.00-60.00  sec  63.8 MBytes  8.92 Mbits/sec                  receiver
[ 63]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  286             sender
[ 63]   0.00-60.00  sec  66.7 MBytes  9.33 Mbits/sec                  receiver
[ 65]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  471             sender
[ 65]   0.00-60.00  sec  69.5 MBytes  9.72 Mbits/sec                  receiver
[ 67]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  275             sender
[ 67]   0.00-60.00  sec  65.9 MBytes  9.22 Mbits/sec                  receiver
[ 69]   0.00-60.00  sec  70.0 MBytes  9.79 Mbits/sec   96             sender
[ 69]   0.00-60.00  sec  64.7 MBytes  9.05 Mbits/sec                  receiver
[ 71]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  215             sender
[ 71]   0.00-60.00  sec  67.9 MBytes  9.49 Mbits/sec                  receiver
[ 73]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  297             sender
[ 73]   0.00-60.00  sec  69.8 MBytes  9.76 Mbits/sec                  receiver
[ 75]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  266             sender
[ 75]   0.00-60.00  sec  67.6 MBytes  9.46 Mbits/sec                  receiver
[ 77]   0.00-60.00  sec  81.2 MBytes  11.4 Mbits/sec  690             sender
[ 77]   0.00-60.00  sec  75.7 MBytes  10.6 Mbits/sec                  receiver
[ 79]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  388             sender
[ 79]   0.00-60.00  sec  70.3 MBytes  9.83 Mbits/sec                  receiver
[ 81]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  349             sender
[ 81]   0.00-60.00  sec  68.2 MBytes  9.53 Mbits/sec                  receiver
[ 83]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  356             sender
[ 83]   0.00-60.00  sec  67.4 MBytes  9.43 Mbits/sec                  receiver
[ 85]   0.00-60.00  sec  40.0 MBytes  5.59 Mbits/sec  126             sender
[ 85]   0.00-60.00  sec  34.8 MBytes  4.86 Mbits/sec                  receiver
[ 87]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  338             sender
[ 87]   0.00-60.00  sec  68.2 MBytes  9.54 Mbits/sec                  receiver
[ 89]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  422             sender
[ 89]   0.00-60.00  sec  70.4 MBytes  9.84 Mbits/sec                  receiver
[ 91]   0.00-60.00  sec  78.8 MBytes  11.0 Mbits/sec  619             sender
[ 91]   0.00-60.00  sec  73.5 MBytes  10.3 Mbits/sec                  receiver
[ 93]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  388             sender
[ 93]   0.00-60.00  sec  70.4 MBytes  9.84 Mbits/sec                  receiver
[ 95]   0.00-60.00  sec  56.2 MBytes  7.86 Mbits/sec  179             sender
[ 95]   0.00-60.00  sec  51.3 MBytes  7.17 Mbits/sec                  receiver
[ 97]   0.00-60.00  sec  77.5 MBytes  10.8 Mbits/sec  502             sender
[ 97]   0.00-60.00  sec  72.3 MBytes  10.1 Mbits/sec                  receiver
[ 99]   0.00-60.00  sec  32.5 MBytes  4.54 Mbits/sec  179             sender
[ 99]   0.00-60.00  sec  27.0 MBytes  3.78 Mbits/sec                  receiver
[101]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  404             sender
[101]   0.00-60.00  sec  70.4 MBytes  9.85 Mbits/sec                  receiver
[103]   0.00-60.00  sec  57.5 MBytes  8.04 Mbits/sec  185             sender
[103]   0.00-60.00  sec  51.6 MBytes  7.22 Mbits/sec                  receiver
[105]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  497             sender
[105]   0.00-60.00  sec  70.5 MBytes  9.86 Mbits/sec                  receiver
[107]   0.00-60.00  sec  83.8 MBytes  11.7 Mbits/sec  922             sender
[107]   0.00-60.00  sec  77.8 MBytes  10.9 Mbits/sec                  receiver
[109]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  250             sender
[109]   0.00-60.00  sec  66.8 MBytes  9.33 Mbits/sec                  receiver
[111]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  353             sender
[111]   0.00-60.00  sec  69.1 MBytes  9.65 Mbits/sec                  receiver
[113]   0.00-60.00  sec  70.0 MBytes  9.79 Mbits/sec  185             sender
[113]   0.00-60.00  sec  64.7 MBytes  9.04 Mbits/sec                  receiver
[115]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  242             sender
[115]   0.00-60.00  sec  68.0 MBytes  9.51 Mbits/sec                  receiver
[117]   0.00-60.00  sec  95.0 MBytes  13.3 Mbits/sec  1114             sender
[117]   0.00-60.00  sec  89.4 MBytes  12.5 Mbits/sec                  receiver
[119]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  437             sender
[119]   0.00-60.00  sec  71.0 MBytes  9.93 Mbits/sec                  receiver
[121]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  409             sender
[121]   0.00-60.00  sec  69.9 MBytes  9.78 Mbits/sec                  receiver
[123]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  316             sender
[123]   0.00-60.00  sec  68.1 MBytes  9.52 Mbits/sec                  receiver
[125]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  246             sender
[125]   0.00-60.00  sec  67.1 MBytes  9.38 Mbits/sec                  receiver
[127]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  201             sender
[127]   0.00-60.00  sec  65.7 MBytes  9.19 Mbits/sec                  receiver
[129]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  193             sender
[129]   0.00-60.00  sec  66.5 MBytes  9.29 Mbits/sec                  receiver
[131]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  193             sender
[131]   0.00-60.00  sec  66.2 MBytes  9.26 Mbits/sec                  receiver
[133]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  331             sender
[133]   0.00-60.00  sec  69.1 MBytes  9.66 Mbits/sec                  receiver
[135]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  344             sender
[135]   0.00-60.00  sec  68.5 MBytes  9.58 Mbits/sec                  receiver
[137]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  322             sender
[137]   0.00-60.00  sec  70.0 MBytes  9.79 Mbits/sec                  receiver
[139]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  433             sender
[139]   0.00-60.00  sec  70.4 MBytes  9.84 Mbits/sec                  receiver
[141]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  266             sender
[141]   0.00-60.00  sec  66.9 MBytes  9.36 Mbits/sec                  receiver
[143]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  402             sender
[143]   0.00-60.00  sec  69.9 MBytes  9.77 Mbits/sec                  receiver
[145]   0.00-60.00  sec  78.8 MBytes  11.0 Mbits/sec  648             sender
[145]   0.00-60.00  sec  73.1 MBytes  10.2 Mbits/sec                  receiver
[147]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  287             sender
[147]   0.00-60.00  sec  67.7 MBytes  9.46 Mbits/sec                  receiver
[149]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  330             sender
[149]   0.00-60.00  sec  69.4 MBytes  9.71 Mbits/sec                  receiver
[151]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  220             sender
[151]   0.00-60.00  sec  66.6 MBytes  9.31 Mbits/sec                  receiver
[153]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  345             sender
[153]   0.00-60.00  sec  67.1 MBytes  9.39 Mbits/sec                  receiver
[155]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  233             sender
[155]   0.00-60.00  sec  67.3 MBytes  9.42 Mbits/sec                  receiver
[157]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  508             sender
[157]   0.00-60.00  sec  69.7 MBytes  9.74 Mbits/sec                  receiver
[159]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  425             sender
[159]   0.00-60.00  sec  71.1 MBytes  9.95 Mbits/sec                  receiver
[161]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  443             sender
[161]   0.00-60.00  sec  70.0 MBytes  9.79 Mbits/sec                  receiver
[163]   0.00-60.00  sec  97.5 MBytes  13.6 Mbits/sec  1582             sender
[163]   0.00-60.00  sec  91.5 MBytes  12.8 Mbits/sec                  receiver
[165]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  269             sender
[165]   0.00-60.00  sec  68.8 MBytes  9.61 Mbits/sec                  receiver
[167]   0.00-60.00  sec  47.5 MBytes  6.64 Mbits/sec  127             sender
[167]   0.00-60.00  sec  42.0 MBytes  5.87 Mbits/sec                  receiver
[169]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  333             sender
[169]   0.00-60.00  sec  70.4 MBytes  9.84 Mbits/sec                  receiver
[171]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  227             sender
[171]   0.00-60.00  sec  65.4 MBytes  9.14 Mbits/sec                  receiver
[173]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  491             sender
[173]   0.00-60.00  sec  71.2 MBytes  9.95 Mbits/sec                  receiver
[175]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  417             sender
[175]   0.00-60.00  sec  69.8 MBytes  9.76 Mbits/sec                  receiver
[177]   0.00-60.00  sec  80.0 MBytes  11.2 Mbits/sec  734             sender
[177]   0.00-60.00  sec  74.6 MBytes  10.4 Mbits/sec                  receiver
[179]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  497             sender
[179]   0.00-60.00  sec  70.7 MBytes  9.89 Mbits/sec                  receiver
[181]   0.00-60.00  sec  62.5 MBytes  8.74 Mbits/sec  231             sender
[181]   0.00-60.00  sec  57.7 MBytes  8.06 Mbits/sec                  receiver
[183]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  412             sender
[183]   0.00-60.00  sec  70.4 MBytes  9.84 Mbits/sec                  receiver
[185]   0.00-60.00  sec  76.2 MBytes  10.7 Mbits/sec  305             sender
[185]   0.00-60.00  sec  70.2 MBytes  9.82 Mbits/sec                  receiver
[187]   0.00-60.00  sec  37.5 MBytes  5.24 Mbits/sec   86             sender
[187]   0.00-60.00  sec  32.0 MBytes  4.47 Mbits/sec                  receiver
[189]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  256             sender
[189]   0.00-60.00  sec  66.3 MBytes  9.27 Mbits/sec                  receiver
[191]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  431             sender
[191]   0.00-60.00  sec  69.5 MBytes  9.72 Mbits/sec                  receiver
[193]   0.00-60.00  sec  78.8 MBytes  11.0 Mbits/sec  590             sender
[193]   0.00-60.00  sec  73.7 MBytes  10.3 Mbits/sec                  receiver
[195]   0.00-60.00  sec  71.2 MBytes  9.96 Mbits/sec  227             sender
[195]   0.00-60.00  sec  65.8 MBytes  9.21 Mbits/sec                  receiver
[197]   0.00-60.00  sec  73.8 MBytes  10.3 Mbits/sec  320             sender
[197]   0.00-60.00  sec  68.5 MBytes  9.57 Mbits/sec                  receiver
[199]   0.00-60.00  sec  33.8 MBytes  4.72 Mbits/sec  153             sender
[199]   0.00-60.00  sec  28.3 MBytes  3.96 Mbits/sec                  receiver
[201]   0.00-60.00  sec  75.0 MBytes  10.5 Mbits/sec  404             sender
[201]   0.00-60.00  sec  69.5 MBytes  9.71 Mbits/sec                  receiver
[203]   0.00-60.00  sec  72.5 MBytes  10.1 Mbits/sec  226             sender
[203]   0.00-60.00  sec  67.2 MBytes  9.39 Mbits/sec                  receiver
[SUM]   0.00-60.00  sec  7.03 GBytes  1.01 Gbits/sec  37331             sender
[SUM]   0.00-60.00  sec  6.50 GBytes   931 Mbits/sec                  receiver
-- so for this case cubic and PK3C working similar (cubic better and more smooth).


One more case of 10 connections from USA to Israel (like reallife example of bad internet channel):
cubic using kernel4:
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec  7.50 MBytes  1.05 Mbits/sec   89             sender
[  5]   0.00-60.00  sec  2.35 MBytes   328 Kbits/sec                  receiver
[  7]   0.00-60.00  sec  10.0 MBytes  1.40 Mbits/sec   55             sender
[  7]   0.00-60.00  sec  5.14 MBytes   719 Kbits/sec                  receiver
[  9]   0.00-60.00  sec  7.50 MBytes  1.05 Mbits/sec   97             sender
[  9]   0.00-60.00  sec  2.69 MBytes   376 Kbits/sec                  receiver
[ 11]   0.00-60.00  sec  8.75 MBytes  1.22 Mbits/sec   71             sender
[ 11]   0.00-60.00  sec  3.44 MBytes   481 Kbits/sec                  receiver
[ 13]   0.00-60.00  sec  10.0 MBytes  1.40 Mbits/sec   68             sender
[ 13]   0.00-60.00  sec  4.84 MBytes   676 Kbits/sec                  receiver
[ 15]   0.00-60.00  sec  8.75 MBytes  1.22 Mbits/sec   85             sender
[ 15]   0.00-60.00  sec  3.09 MBytes   432 Kbits/sec                  receiver
[ 17]   0.00-60.00  sec  8.75 MBytes  1.22 Mbits/sec   73             sender
[ 17]   0.00-60.00  sec  3.81 MBytes   532 Kbits/sec                  receiver
[ 19]   0.00-60.00  sec  7.50 MBytes  1.05 Mbits/sec   90             sender
[ 19]   0.00-60.00  sec  2.68 MBytes   375 Kbits/sec                  receiver
[ 21]   0.00-60.00  sec  8.75 MBytes  1.22 Mbits/sec   75             sender
[ 21]   0.00-60.00  sec  3.19 MBytes   445 Kbits/sec                  receiver
[ 23]   0.00-60.00  sec  11.2 MBytes  1.57 Mbits/sec   57             sender
[ 23]   0.00-60.00  sec  5.32 MBytes   744 Kbits/sec                  receiver
[SUM]   0.00-60.00  sec  88.8 MBytes  12.4 Mbits/sec  760             sender
[SUM]   0.00-60.00  sec  36.5 MBytes  5.11 Mbits/sec                  receiver

PK3C using kernel4:
[ ID] Interval           Transfer     Bandwidth       Retr
[  5]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  428             sender
[  5]   0.00-30.21  sec  2.07 MBytes   575 Kbits/sec                  receiver
[  7]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  375             sender
[  7]   0.00-30.21  sec  1.68 MBytes   468 Kbits/sec                  receiver
[  9]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  369             sender
[  9]   0.00-30.21  sec  1.83 MBytes   509 Kbits/sec                  receiver
[ 11]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  362             sender
[ 11]   0.00-30.21  sec  1.56 MBytes   432 Kbits/sec                  receiver
[ 13]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  404             sender
[ 13]   0.00-30.21  sec  1.78 MBytes   495 Kbits/sec                  receiver
[ 15]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  374             sender
[ 15]   0.00-30.21  sec  1.53 MBytes   425 Kbits/sec                  receiver
[ 17]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  429             sender
[ 17]   0.00-30.21  sec  1.87 MBytes   519 Kbits/sec                  receiver
[ 19]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  359             sender
[ 19]   0.00-30.21  sec  1.60 MBytes   444 Kbits/sec                  receiver
[ 21]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  484             sender
[ 21]   0.00-30.21  sec  2.68 MBytes   744 Kbits/sec                  receiver
[ 23]   0.00-30.21  sec  7.50 MBytes  2.08 Mbits/sec  364             sender
[ 23]   0.00-30.21  sec  1.58 MBytes   439 Kbits/sec                  receiver
[SUM]   0.00-30.21  sec  75.0 MBytes  20.8 Mbits/sec  3948             sender
[SUM]   0.00-30.21  sec  18.2 MBytes  5.05 Mbits/sec                  receiver
-- seeing that similar result, but PK3C forcing higher amount of Retr (means PK3C could work faster if Cubic would not work the best for this case).


And similar for 3 connections (from USA to Israel):
PK3C:
[ ID] Interval           Transfer     Bandwidth       Retr
[  5]   0.00-30.19  sec  11.2 MBytes  3.13 Mbits/sec  291             sender
[  5]   0.00-30.19  sec  5.81 MBytes  1.61 Mbits/sec                  receiver
[  7]   0.00-30.19  sec  12.5 MBytes  3.47 Mbits/sec  340             sender
[  7]   0.00-30.19  sec  6.63 MBytes  1.84 Mbits/sec                  receiver
[  9]   0.00-30.19  sec  11.2 MBytes  3.13 Mbits/sec  287             sender
[  9]   0.00-30.19  sec  5.61 MBytes  1.56 Mbits/sec                  receiver
[SUM]   0.00-30.19  sec  35.0 MBytes  9.73 Mbits/sec  918             sender
[SUM]   0.00-30.19  sec  18.1 MBytes  5.02 Mbits/sec                  receiver
    -- and from receiver point of view slow-start worked well (stabilization happened in about 5 seconds and without pushing too much data to the channel):
[ ID] Interval           Transfer     Bandwidth
[  5]   0.00-1.00   sec   143 KBytes  1.17 Mbits/sec                  
[  7]   0.00-1.00   sec  65.0 KBytes   533 Kbits/sec                  
[  9]   0.00-1.00   sec  83.4 KBytes   683 Kbits/sec                  
[SUM]   0.00-1.00   sec   291 KBytes  2.39 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   1.00-2.00   sec   192 KBytes  1.58 Mbits/sec                  
[  7]   1.00-2.00   sec   117 KBytes   962 Kbits/sec                  
[  9]   1.00-2.00   sec   218 KBytes  1.78 Mbits/sec                  
[SUM]   1.00-2.00   sec   527 KBytes  4.32 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   2.00-3.00   sec   232 KBytes  1.90 Mbits/sec                  
[  7]   2.00-3.00   sec   311 KBytes  2.55 Mbits/sec                  
[  9]   2.00-3.00   sec   266 KBytes  2.18 Mbits/sec                  
[SUM]   2.00-3.00   sec   809 KBytes  6.63 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   3.00-4.00   sec   119 KBytes   973 Kbits/sec                  
[  7]   3.00-4.00   sec   206 KBytes  1.69 Mbits/sec                  
[  9]   3.00-4.00   sec   133 KBytes  1.09 Mbits/sec                  
[SUM]   3.00-4.00   sec   458 KBytes  3.75 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   4.00-5.00   sec   223 KBytes  1.83 Mbits/sec                  
[  7]   4.00-5.00   sec   291 KBytes  2.39 Mbits/sec                  
[  9]   4.00-5.00   sec   144 KBytes  1.18 Mbits/sec                  
[SUM]   4.00-5.00   sec   659 KBytes  5.40 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   5.00-6.00   sec   279 KBytes  2.28 Mbits/sec                  
[  7]   5.00-6.00   sec   356 KBytes  2.92 Mbits/sec                  
[  9]   5.00-6.00   sec   242 KBytes  1.98 Mbits/sec                  
[SUM]   5.00-6.00   sec   877 KBytes  7.18 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   6.00-7.00   sec   187 KBytes  1.53 Mbits/sec                  
[  7]   6.00-7.00   sec   270 KBytes  2.21 Mbits/sec                  
[  9]   6.00-7.00   sec   167 KBytes  1.37 Mbits/sec                  
[SUM]   6.00-7.00   sec   624 KBytes  5.11 Mbits/sec
...

, and similar for cubic:
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-60.00  sec  17.5 MBytes  2.45 Mbits/sec   43             sender
[  5]   0.00-60.00  sec  12.0 MBytes  1.68 Mbits/sec                  receiver
[  7]   0.00-60.00  sec  12.5 MBytes  1.75 Mbits/sec   95             sender
[  7]   0.00-60.00  sec  6.78 MBytes   948 Kbits/sec                  receiver
[  9]   0.00-60.00  sec  23.8 MBytes  3.32 Mbits/sec   18             sender
[  9]   0.00-60.00  sec  18.0 MBytes  2.51 Mbits/sec                  receiver
[SUM]   0.00-60.00  sec  53.8 MBytes  7.51 Mbits/sec  156             sender
[SUM]   0.00-60.00  sec  36.7 MBytes  5.14 Mbits/sec                  receiver
   -- so cubic worked worse regarding being less smooth, see 948 KBits above and for PK3C all results were higher than 1.56 MBits/sec, and slow start for cubic:
[ ID] Interval           Transfer     Bandwidth
[  5]   0.00-1.00   sec   181 KBytes  1.48 Mbits/sec                  
[  7]   0.00-1.00   sec   148 KBytes  1.22 Mbits/sec                  
[  9]   0.00-1.00   sec   126 KBytes  1.03 Mbits/sec                  
[SUM]   0.00-1.00   sec   455 KBytes  3.73 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   1.00-2.00   sec   277 KBytes  2.27 Mbits/sec                  
[  7]   1.00-2.00   sec   160 KBytes  1.31 Mbits/sec                  
[  9]   1.00-2.00   sec   205 KBytes  1.68 Mbits/sec                  
[SUM]   1.00-2.00   sec   642 KBytes  5.26 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   2.00-3.00   sec   262 KBytes  2.14 Mbits/sec                  
[  7]   2.00-3.00   sec   156 KBytes  1.27 Mbits/sec                  
[  9]   2.00-3.00   sec   221 KBytes  1.81 Mbits/sec                  
[SUM]   2.00-3.00   sec   638 KBytes  5.22 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   3.00-4.00   sec   255 KBytes  2.09 Mbits/sec                  
[  7]   3.00-4.00   sec   153 KBytes  1.25 Mbits/sec                  
[  9]   3.00-4.00   sec   218 KBytes  1.78 Mbits/sec                  
[SUM]   3.00-4.00   sec   625 KBytes  5.12 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[  5]   4.00-5.00   sec   225 KBytes  1.84 Mbits/sec                  
[  7]   4.00-5.00   sec   164 KBytes  1.34 Mbits/sec                  
[  9]   4.00-5.00   sec   235 KBytes  1.92 Mbits/sec                  
[SUM]   4.00-5.00   sec   624 KBytes  5.11 Mbits/sec

, and 3 connections from USA to Israel for kernel3 with PK3C:
[ ID] Interval           Transfer     Bandwidth       Retr
[  5]   0.00-30.21  sec  15.0 MBytes  4.16 Mbits/sec  8449             sender
[  5]   0.00-30.21  sec  11.6 MBytes  3.21 Mbits/sec                  receiver
[  7]   0.00-30.21  sec  8.75 MBytes  2.43 Mbits/sec  4395             sender
[  7]   0.00-30.21  sec  5.03 MBytes  1.40 Mbits/sec                  receiver
[  9]   0.00-30.21  sec  3.75 MBytes  1.04 Mbits/sec  1120             sender
[  9]   0.00-30.21  sec   464 KBytes   126 Kbits/sec                  receiver
[SUM]   0.00-30.21  sec  27.5 MBytes  7.64 Mbits/sec  13964             sender
[SUM]   0.00-30.21  sec  17.0 MBytes  4.73 Mbits/sec                  receiver



Conclusion.
The PK3C optimized for video streams. It is known that HD video streams are at least 5 Mbps each, so if PK3C gives better results for such streams, than cubic, like ~5 Mbits/sec streams for 20 streams inside 100 MBits channel, then it works as expected (and likely about 16% better, than Cubic).
However, PK3C could be used for servers only (like for data providers servers) and unlikely that it can be used everywhere for every device (since it is less universal, than cubic, and behavior for very low rate connections is bad for PK3C comparing with Cubic).
However, for data (or video) providers the ~ 16% benefit could be sensitive (say 16% cheaper hardware or 16% better customers experience).
The last example above about bad connection from USA to Israel shown that PK3C allows 3 users with 1.56 MBits/sec connection for the same channel where cubic allows only 2 such users).



