confirm false
cd component2
ls -l
trace message_count_ changed : 32 4 : message_count_ : printTrace
sethandler 0 ae ac
printwatchpoint 0
printtrace 0
run 3000ps
wl
print message_count_
printwatchpoint 0
printtrace 0
run 3000ps
wl
print message_count_
printwatchpoint 0
printtrace 0
shutd
