# ======================
# Stage 1: Build
# ======================
FROM golang:1.24-alpine AS builder

WORKDIR /build

# Cache dependencies
COPY go.mod go.sum ./
RUN go mod download

# Copy source code
COPY . .

# Build static binary (modernc.org/sqlite is pure Go, no CGo needed)
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -ldflags="-s -w" -o /build/drfrake-backend .

# Build migration script
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -ldflags="-s -w" -o /build/migrate_db ./migrate_db.go

# ======================
# Stage 2: Runtime
# ======================
FROM alpine:3.19

# CA certificates for HTTPS calls to YooKassa and Outline APIs
RUN apk add --no-cache ca-certificates tzdata

WORKDIR /app

# Copy built binaries
COPY --from=builder /build/drfrake-backend /app/drfrake-backend
COPY --from=builder /build/migrate_db /app/migrate_db

# Copy entrypoint script
COPY entrypoint.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh

# Create data directory for SQLite
RUN mkdir -p /data

# Default environment variables
ENV PORT=":8080"
ENV DB_PATH="/data/server.db"

EXPOSE 8080

VOLUME ["/data"]

ENTRYPOINT ["/app/entrypoint.sh"]
