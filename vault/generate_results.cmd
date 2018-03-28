set LOCAL_PATH=c:\program files (x86)\Apache Software Foundation\Apache2.2\bin

set server_url=http://192.168.56.1:9000/
set run_time=240
set number_of_requests=60000
set number_of_concurrent_users=60

set number_of_concurrent_users=10
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >  perf_test_results.txt

set number_of_concurrent_users=50
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=100
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=200
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=500
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=1000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=2000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=3000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=4000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=5000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=6000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=7000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=8000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=9000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

set number_of_concurrent_users=10000
"%LOCAL_PATH%\ab" -t %run_time% -n %number_of_requests% -c %number_of_concurrent_users% %server_url% >>  perf_test_results.txt

