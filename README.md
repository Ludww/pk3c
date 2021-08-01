This is TCP/IP congestion control for video streaming/data servers.
First, read "understanding": https://granulate.io/understanding-congestion-control/

How to improve for free quality of network connections?
How to make upload data faster (up to 5 times faster!) for your workstation?
How to make download data faster for your servers?
If you don't know what all this about, then read first about Cubic congestion control ( https://en.wikipedia.org/wiki/CUBIC_TCP ) and then about similar Google project BBR that is more in common with this project (than if comparing with Cubic).

These graphs shows upload test by speed.io (both works and working differently):

, the PK3C makes decision about best upload speed faster (without trying too high):

![Alt text](/imgs/PK3CUpload.gif?raw=true "PK3C Upload graph")

, and for one connection the Cubic is a little bit faster:

![Alt text](/imgs/CubicUpload.gif?raw=true "Cubic Upload graph")

!!! The most funny thing that this PK3C is sending data more aggressively than Cubic, so if few Cubic connections and few PK3C connections
share the same link, then PK3C wins (about 5 times wins!)!
This is the answer to question why Google uses BBR instead of Cubic (because they wants their traffic to be downloadable faster than from other server that are uses non-BBR).
Both it means that this PK3C project could be usefull not only for servers, but both for your workstation (because both connections initiation goint to be faster and both upload data from your workstation would be faster).

Take in mind that this PK3C project has nothing in common with any other versions of PCC project, so the new name PK3C suppose to show that
it is based on original https://github.com/PCCproject/PCC-Kernel project that is under BSD/Gplv2 , so original PCC-Kernel included in subdir PCC-Kernel for history reasons
(this PK3C based on original PCC opensource code (included in ./PCC-Kernel/* );
you may find some other related to PCC projects like some userspace control utility, see https://github.com/PCCproject/PCC-Uspace , and some links to original
papers there in PCC-Uspace that are relevant both for PCC-Kernel and regarding original parts taken from PCC-Kernel for this PK3C too,
but this PK3C project is independent and currently not compatible with this PCC-Uspace utility; generally, see LICENSE and ask questions inside github or privately
to ludwigschapiro@gmail.com , so I'm Ludwig and I'm author of diff from PCC-Kernel to
this PK3C;
found one other similar project that is based on original PCC-Kernel: https://github.com/KaiwenZha/PCC-Vivace
, but as far as I know this PK3C gives better results than PCC-Vivace ).

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

, and as root run these five commands (after module loaded and for making all outside connections to use PK3C) :


sudo sysctl -w net.core.wmem_max=8194300

sudo sysctl -w net.ipv4.tcp_wmem="4096 8194300 8194300"

sudo sysctl -w net.core.rmem_max=4258291

sudo sysctl -w net.ipv4.tcp_rmem="4096 8194300 8194300"

echo "pk3c" > /proc/sys/net/ipv4/tcp_congestion_control


Enjoy!
Now the speed of your internet and browsing should be much faster!!!
This module improves speed of uploading (like speed of sending data from your machine to somewhere), but still it improves "feeling" of fast internet for local workstation too, because it improves slow-start and because then internet connections of other users starts working slower than yours connections (means it improves the speed of connection-initiation and both makes your connections more aggressively comparing to other regular user connection like could be situation when you use 4/5 of channel and other user only 1/5 of the same channel);
and both it gives higher overall speed of data upload from machine where you try it.
However, if you try one big file upload test, it could be both faster or a little bit slower comparing to default "Cubic"
(but take in mind that one file upload test is nothing and instead you should try 100 or 1000 files uploading simultaneously and then you would see the difference,
or for comparing with cubic try simple 5 connections together running "/bin/bash ./a.sh" where "cat ./a.sh" and server is remote server, ex. server in Google Cloud:

time scp ./IMG_7700.MOV user@server:~/1 &

time scp ./IMG_7700.MOV user@server:~/2 &

time scp ./IMG_7700.MOV user@server:~/3 &

time scp ./IMG_7700.MOV user@server:~/4 &

time scp ./IMG_7700.MOV user@server:~/5 &

- with PK3C this should work about %20 faster, than if with default Cubic,
and if running together 5 uploads with PK3C and other similar 5 with Cubic, then PK3C should win about 5 times in speed!
).


Advantages of PK3C.
Generally, there are different similar kernel modules for TCP/IP congestion (like see http://web.cs.wpi.edu/~claypool/papers/tcp-sat-pam-21/camera-draft.pdf
for comparing behavior of one connection using Cubic, BBR, Hybla and PCC kernel module).
And BBR (from Google) is similar like PCC or like this PK3C (it is rate based and Cubic is Window based).
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
See results of testing: https://github.com/Ludww/pk3c/blob/main/testing.txt




Conclusion.
The PK3C optimized for video streams. It is known that HD video streams are at least 5 Mbps each, so if PK3C gives better results for such streams, than Cubic, like ~5 Mbits/sec streams for 20 streams inside 100 MBits channel, then it works as expected (and faster, than Cubic).
However, PK3C could be used both for servers and for Linux workstations (it is a huge benefit for data providers servers and both for regular users).
The only disadvantage of PK3C is that it doesn't work good with very slow connections (like less than 250KBits/sec).
However, for data (or video) providers the benefit could be sensitive (say 40% cheaper hardware or 40% better customers experience).
The big advantage of PK3C that it can work good with very noisy (and high packets loss) connections, and Cubic not at all.
See results of testing: https://github.com/Ludww/pk3c/blob/main/testing.txt
