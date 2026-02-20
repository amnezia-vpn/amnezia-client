# ======================
# Stage 1: Build
# ======================
FROM golang:1.21-alpine AS builder

WORKDIR /build

# Cache dependencies
COPY go.mod go.sum ./
RUN go mod download

# Copy source code
COPY . .

# Build static binary (modernc.org/sqlite is pure Go, no CGo needed)
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -ldflags="-s -w" -o /build/drfrake-backend .

# ======================
# Stage 2: Runtime
# ======================
FROM alpine:3.19

# CA certificates for HTTPS calls to YooKassa and Outline APIs
RUN apk add --no-cache ca-certificates tzdata

WORKDIR /app

# Copy built binary
COPY --from=builder /build/drfrake-backend /app/drfrake-backend

# Create data directory for SQLite
RUN mkdir -p /data

# Default environment variables
ENV PORT=":8080"
ENV DB_PATH="/data/server.db"

EXPOSE 8080

VOLUME ["/data"]

ENTRYPOINT ["/app/drfrake-backend"]
