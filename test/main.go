package main

import (
	//	"fmt"
	"net"
	"os"
	"time"
)

func main() {
	strEcho := "Hello!\n"
	for i := 0; i < 1000; i++ {
		time.Sleep(100 * time.Millisecond)
		go func() {
			tcpAddr, err := net.ResolveTCPAddr("tcp", "127.0.0.1:6000")
			if err != nil {
				println("ResolveTCPAddr failed:", err.Error())
				os.Exit(1)
			}

			conn, err := net.DialTCP("tcp", nil, tcpAddr)
			if err != nil {
				println("Dial failed:", err.Error())
				os.Exit(1)
			}
			for {
				_, err = conn.Write([]byte(strEcho))
				if err != nil {
					println("Write to server failed:", err.Error())
					os.Exit(1)
				}

				//println("write to server = ", strEcho)

				time.Sleep(5 * time.Second)

				reply := make([]byte, 8192)

				_, err = conn.Read(reply)
				if err != nil {
					println("Read from server failed:", err.Error())
					os.Exit(1)
				}

				//println("reply from server=", string(reply))
			}
		}()
	}
	time.Sleep(3600 * time.Second)
}
