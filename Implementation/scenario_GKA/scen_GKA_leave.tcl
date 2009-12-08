set ns [new Simulator]
set nf [open out_GKA.nam w]
$ns namtrace-all $nf

proc finish {} {
        global ns nf
        $ns flush-trace
        close $nf
        exec nam out_GKA.nam &
        exit 0
}

# Topology Configuration

for {set i 0} {$i < 20} {incr i} {
  set n($i) [$ns node]
  set m($i) [$ns node]
}

$ns duplex-link $n(0) $n(1) 1Mb 10ms DropTail
$ns duplex-link $n(0) $n(2) 1Mb 10ms DropTail

$ns duplex-link $m(0) $m(1) 1Mb 10ms DropTail
$ns duplex-link $m(0) $m(2) 1Mb 10ms DropTail

for {set i 2} {$i < 9} {incr i} {
  $ns duplex-link $n(1) $n([expr ($i+1)]) 1Mb 10ms DropTail
  $ns duplex-link $m(1) $m([expr ($i+1)]) 1Mb 10ms DropTail
}

for {set i 9} {$i < 19} {incr i} {
  $ns duplex-link $n(2) $n([expr ($i+1)]) 1Mb 10ms DropTail
  $ns duplex-link $m(2) $m([expr ($i+1)]) 1Mb 10ms DropTail
}

# link between leaders
$ns duplex-link $m(0) $n(0) 1Mb 10ms DropTail


# Create GKA agents and attach one of them to each node

for {set i 0} {$i < 10} {incr i} {
  set gka_n($i) [new Agent/GKA]
  set gka_m($i) [new Agent/GKA]
}

for {set i 0} {$i < 10} {incr i} {
  $ns attach-agent $n($i) $gka_n($i)
  $ns attach-agent $m($i) $gka_m($i)
}


# Simulation 

for {set i 0} {$i < 9} {incr i} {
$ns at [expr (0.5+$i*0.5)] "$gka_n($i) join $gka_n(9)"
$ns at [expr (0.5+$i*0.5)] "$gka_m($i) join $gka_m(9)"
}

$ns at 3.0 "$gka_n(1) leave"

$ns at 5.0 "$gka_n(0) merge $gka_m(0)"
$ns at 10.0 "finish"
$ns run
