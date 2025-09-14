**1. 3 TCP-сессии с установлением соединения (SYN, SYN-ACK, ACK):**  
`sudo tcpdump -i any -c 3 'tcp[tcpflags] & tcp-syn != 0'`

**2. DNS-запрос к домену google.com.:**
`sudo tcpdump -i any port 53 and host google.com`

**3. Любой ICMP-пакет (пинг).**
`sudo tcpdump -i any -c 1 icmp`

**4. Самый большой по размеру пакет в дампе.**
`tcpdump -r ipv4frags.pcap -n | awk '/length/ {print $NF, $0}' | sort -nr | head -n 1 | cut -d ' ' -f 2-`