//go:build ignore

package main

import (
	"database/sql"
	"fmt"
	"log"
	"os"

	_ "github.com/mattn/go-sqlite3"
)

func main() {
	// Docker mounts /data to a local volume or path.
	// The user running go locally to clean it up might need to target the local mount or we just compile it into the container.
	// Let's just create a small go script that the user can run via docker exec since go IS installed in the builder container.
	dbPath := "/data/server.db"
	if len(os.Args) > 1 {
		dbPath = os.Args[1]
	}

	db, err := sql.Open("sqlite3", dbPath)
	if err != nil {
		log.Fatalf("Failed to open db: %v", err)
	}
	defer db.Close()

	res, err := db.Exec("DELETE FROM access_keys;")
	if err != nil {
		log.Fatalf("Failed to execute: %v", err)
	}

	count, _ := res.RowsAffected()
	fmt.Printf("Cleared %d access keys.\n", count)
}
