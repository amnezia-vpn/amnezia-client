package main

import (
	"bytes"
	"fmt"
	"io"
	"net/http"
	"net/url"
)

func main() {
	baseURL := "http://138.124.101.69:2096/b4NFYTUCTERsAG4d83"

	endpoints := []string{
		"/login",
		"login",
		"/panel/login",
		"/api/login",
	}

	payload := []byte(`{"username":"admin","password":"admin"}`)

	for _, ep := range endpoints {

		targetPath, _ := url.JoinPath(baseURL, ep)

		fmt.Printf("\nTesting endpoint: %s\n", targetPath)

		resp, err := http.Post(targetPath, "application/json", bytes.NewBuffer(payload))
		if err != nil {
			fmt.Println("Error:", err)
			continue
		}

		body, _ := io.ReadAll(resp.Body)
		resp.Body.Close()

		fmt.Printf("Status: %s\n", resp.Status)
		fmt.Printf("Body: %s\n", string(body))
	}
}
