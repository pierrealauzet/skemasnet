set ns [new Simulator]
set nf [open out_SKMN_join_50_nodes.nam w]
#$ns namtrace-all $nf
$ns trace-all $nf

proc finish {} {
        global ns nf
        $ns flush-trace
        close $nf
#        exec nam out_SKMN_leave_20_nodes.nam &
        exit 0
}

# Topology Configuration

#for {set i 0} {$i < 10} {incr i} {
#for {set i 0} {$i < 20} {incr i} {
#for {set i 0} {$i < 30} {incr i} {
#for {set i 0} {$i < 40} {incr i} {
for {set i 0} {$i < 50} {incr i} {
  set n($i) [$ns node]
  set m($i) [$ns node]
}

$ns duplex-link $n(0) $n(1) 1Mb 10ms DropTail
$ns duplex-link $n(0) $n(2) 1Mb 10ms DropTail

$ns duplex-link $m(0) $m(1) 1Mb 10ms DropTail
$ns duplex-link $m(0) $m(2) 1Mb 10ms DropTail

#for {set i 2} {$i < 5} {incr i} {
#for {set i 2} {$i < 9} {incr i} {
#for {set i 2} {$i < 15} {incr i} {
#for {set i 2} {$i < 19} {incr i} {
for {set i 2} {$i < 25} {incr i} {
  $ns duplex-link $n(1) $n([expr ($i+1)]) 1Mb 10ms DropTail
  $ns duplex-link $m(1) $m([expr ($i+1)]) 1Mb 10ms DropTail
}

#for {set i 5} {$i < 9} {incr i} {
#for {set i 9} {$i < 19} {incr i} {
#for {set i 15} {$i < 29} {incr i} {
#for {set i 19} {$i < 39} {incr i} {
for {set i 25} {$i < 49} {incr i}  {
  $ns duplex-link $n(2) $n([expr ($i+1)]) 1Mb 10ms DropTail
  $ns duplex-link $m(2) $m([expr ($i+1)]) 1Mb 10ms DropTail
}


# link between leaders	- not considered in join and leaving
#$ns duplex-link $m(0) $n(0) 1Mb 10ms DropTail


# Create SKMN agents and attach one of them to each node

#for {set i 0} {$i < 10} {incr i} {
#for {set i 0} {$i < 20} {incr i} {
#for {set i 0} {$i < 30} {incr i} {
#for {set i 0} {$i < 40} {incr i} {
for {set i 0} {$i < 50} {incr i} {
  set skmn_n($i) [new Agent/SKMN]
  set skmn_m($i) [new Agent/SKMN]
}

#for {set i 0} {$i < 10} {incr i} {
#for {set i 0} {$i < 20} {incr i} {
#for {set i 0} {$i < 30} {incr i} {
#for {set i 0} {$i < 40} {incr i} {
for {set i 0} {$i < 50} {incr i} {
  $ns attach-agent $n($i) $skmn_n($i)
  $ns attach-agent $m($i) $skmn_m($i)
}


# Simulation - Join of all nodes; only one network considered
# Required by leave and merge cases; in the merge case please
# uncomment the line about m() network;

#for {set i 1} {$i < 10} {incr i} {
#for {set i 1} {$i < 20} {incr i} {
#for {set i 1} {$i < 30} {incr i} {
#for {set i 1} {$i < 40} {incr i} {
for {set i 1} {$i < 50} {incr i} {
$ns at [expr ($i*0.5)] "$skmn_n($i) join $skmn_n(0)"
#$ns at [expr ($i*0.5)] "$skmn_m($i) join $skmn_m(0)"
}


# Simulation - One node leaving; only one network considered
# ---> Plese comment this part if unnecessary <---

#$ns at 90.0 "$skmn_n(3) leave"


# Simulation - Merging of 2 networks
# ---> Plese comment this part if unnecessary <---

#$ns at 80.0 "$skmn_n(0) merge $skmn_m(0)"

$ns at 100.0 "finish"
$ns run
