LIMIT=$1
COUNT=0
while [ $COUNT -lt $LIMIT ];
do
  echo -n "$2 : $COUNT : "
  echo "{\"kcm-cmd\":\"send\",\"source-addr\":\"27836800464\",\"destination-addr\":\"27836800465\",\"short-message\":\"This is a test.\"}" | nc -w 2 localhost 9000
  sleep $3
  COUNT=$(($COUNT+1))  
done
