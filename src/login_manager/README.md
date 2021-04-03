# How to configre login server & client
This is a login server designed for multiple users working off of 1 server
## Simple method
Follow step 1 of client and server to install dependencies then  
```make``` this will build the binaries for you
# Server Configuration:
## 1 Install dependencies 
Install mysql Driver for golang  
```go get github.com/go-sql-driver/mysql```
## 2 connect your own database
Modify line 46 in login_server.go to connect to your own Database
## 3 Make sure your database is configured properly
Make sure the database connected has the structure similar to this:  
<img src="login_manager/ComSysDB.png">

## 4 Build
You can either build the go file to a binary using
```go build login_server.go``` or you can just run it with
```go run login_server.go```

# Client Configuration
## 1 Install dependencies 
Install json-c library for parsing server response

**Debian Based Distro:**  
```sudo apt get install libjson-c-dev```
## 2 Build
Build the file using:  
```gcc login_client.c -o login_client -ljson-c```
