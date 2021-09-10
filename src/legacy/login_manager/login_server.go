package main

import (
	"fmt"
	"bufio"
	"io"
	"log"
	"net"
	"strings"
	"database/sql"
	_ "github.com/go-sql-driver/mysql"
	"encoding/json"
)

// define the json data structure
type Response struct{
	Ip string
	Permission string
}

func main() {
	listener, err := net.Listen("tcp", "0.0.0.0:6970")
	if err != nil {
		log.Fatalln(err)
	}

	// when we get to the end of the program automatically close the server
	defer listener.Close()


	for { // while loop cus go has no while loop
		con, err := listener.Accept()
		// handle errors
		if err != nil {
			log.Println(err)
			continue
		}
		// automatic concurrency management for all connections
		go handleClientRequest(con)
	}
}

func handleClientRequest(con net.Conn) {
	log.Printf("Client %s has connected",con.LocalAddr())
	// tim should i open the db here?
	db, err := sql.Open("mysql", "telescope:Q^jM3G^BGAP@2a@tcp(sin1.bordery.net:3306)/telescope")
	if err != nil {
		log.Fatalln(err)
	}
	// at the end of the connection we can just close it
	defer db.Close()
	defer con.Close()

	//new buffer reader
	clientReader := bufio.NewReader(con)

	for {
		// Waiting for the client request
		clientRequest, err := clientReader.ReadString('\n')


		switch err {
		case nil:
			// if there is no error trim spaces and stuff then print out the request
			clientRequest := strings.TrimSpace(clientRequest)

			// split username and password string
			data := strings.Split(clientRequest,",")
			username, password := data[0], data[1]
			log.Print(username,password)
			var response Response

			// to just query if the username exist
			userSQL := fmt.Sprintf("SELECT username,password FROM UserTable WHERE username='%s' AND password=SHA2('%s%s',256)",username,password,username)
			err := db.QueryRow(userSQL).Scan(&response.Ip, &response.Permission)
			if err != nil{
				if err == sql.ErrNoRows {
					response.Ip = "unameerror"
					response.Permission = "none"
				} else {
					log.Fatal(err)
				}
				log.Printf("%s,%s\n", response.Ip,response.Permission)
				// make our struct into a json object
				jsonData, err := json.Marshal(response)
				if err != nil {
					log.Println(err)
				}

				log.Print(string(jsonData))
				// Responding to the client request
				if _, err = con.Write(jsonData); err != nil {
					log.Printf("failed to respond to client: %v\n", err)
				}
			} else {
				SQL := fmt.Sprintf("SELECT ip, permissions FROM ServerUser JOIN Servers S on ServerUser.server_id = S.id JOIN UserTable UT on UT.id = ServerUser.user_id WHERE UT.username ='%s' AND UT.password=SHA2('%s%s',256);", username, password, username)

				err := db.QueryRow(SQL).Scan(&response.Ip, &response.Permission)

				if err != nil {
					if err == sql.ErrNoRows {
						response.Ip = "servererror"
						response.Permission = "none"
					} else {
						log.Fatal(err)
					}
				}
				log.Printf("%s,%s\n", response.Ip,response.Permission)
				jsonData, err := json.Marshal(response)
				if err != nil {
					log.Println(err)
				}

				log.Print(string(jsonData))
				// Responding to the client request
				if _, err = con.Write(jsonData); err != nil {
					log.Printf("failed to respond to client: %v\n", err)
				}
			}

		case io.EOF:
			log.Println("client closed the connection by terminating the process")
			return
		default:
			log.Printf("error: %v\n", err)
			return
		}
	}
}