#!/bin/sh
set -e

# Run database migration
echo "Running database migrations..."
/app/migrate_db

# Start the main backend application
echo "Starting backend server..."
exec /app/drfrake-backend
