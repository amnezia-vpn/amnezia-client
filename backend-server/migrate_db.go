//go:build ignore

package main

import (
	"database/sql"
	"fmt"
	"log"
	"os"

	_ "modernc.org/sqlite"
)

func main() {
	dbPath := "server.db"
	if _, err := os.Stat(dbPath); os.IsNotExist(err) {
		log.Fatal("server.db not found. Run this from the backend-server folder.")
	}

	db, err := sql.Open("sqlite", dbPath)
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// Rename existing columns to the new SSH/Amnezia columns
	queries := []string{
		"ALTER TABLE servers RENAME COLUMN xray_username TO ssh_user;",
		"ALTER TABLE servers RENAME COLUMN xray_password TO ssh_pass;",
		"ALTER TABLE servers RENAME COLUMN xray_settings TO settings;",
		// We can safely drop xray_inbound_id and xray_panel_url, but SQLite ALTER TABLE DROP COLUMN
		// is supported starting from SQLite version 3.35.0 (modernc.org/sqlite should support it).
		"ALTER TABLE servers DROP COLUMN xray_inbound_id;",
		"ALTER TABLE servers DROP COLUMN xray_panel_url;",
	}

	for _, q := range queries {
		_, err := db.Exec(q)
		if err != nil {
			// Ignore errors if columns are already renamed/dropped
			fmt.Printf("Query ran: %s\nResult: %v\n\n", q, err)
		} else {
			fmt.Printf("Success: %s\n", q)
		}
	}

	fmt.Println("Database migration for Amnezia SSH settings completed!")
}
