# Requires 4 threads
confirm false
# 0 watch
cd component0
watch message_count_ changed
run
wl
unwatch 0
cd ..
# 1 trace printStatus
cd component2
trace message_count_ changed : 16 0 : message_count_ : printStatus
setHandler 1 ae
run 1ps
wl
unwatch 1
# 2 trace printTrace
#cd ..
thread 1
cd component5
trace message_count_ changed : 16 5 : message_count_ : printTrace
setHandler 0 ae
run 2000ps
wl
unwatch 0
# 3 trace set
thread 2
cd component8
trace message_count_ changed : 16 2 : message_count_ : set my_id_ 50
setHandler 0 ae
run 1000ps
wl
#print my_id_
unwatch 0
# 4 trace interactive
cd ..
cd component10
trace message_count_ changed : 16 2 : message_count_ : interactive
setHandler 1 ae
run
wl
unwatch 1
# 5 trace shutdown
#cd ..
thread 3
cd component12
trace message_count_ changed : 16 2 : message_count_ : shutdown
run
