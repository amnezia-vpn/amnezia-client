# Build stage
FROM golang:1.22-alpine AS builder

WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download

COPY . .
RUN GOOS=linux go build -o server ./cmd/server

# Run stage
FROM alpine:latest
RUN apk add --no-cache ca-certificates sqlite

WORKDIR /app
COPY --from=builder /app/server .
COPY --from=builder /app/admin ./admin

RUN mkdir -p data
VOLUME ["/app/data"]

EXPOSE 8080
CMD ["./server"]
