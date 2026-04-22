confirm false
# 0 watch
cd c0
cd xorshift
watch w changed
trace w changed : 16 0 : w : checkpoint
run
wl
unwatch 0
unwatch 1
shutd