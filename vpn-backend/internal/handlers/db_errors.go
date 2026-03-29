package handlers

import (
	"errors"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

func isRecordNotFoundError(err error) bool {
	return errors.Is(err, gorm.ErrRecordNotFound)
}

func isDatabaseBusyError(err error) bool {
	if err == nil {
		return false
	}

	message := strings.ToLower(err.Error())
	return strings.Contains(message, "database is locked") ||
		strings.Contains(message, "sqlite_busy") ||
		strings.Contains(message, "sql logic error")
}

func respondDatabaseBusy(c *gin.Context) {
	c.JSON(http.StatusServiceUnavailable, gin.H{
		"error": "database temporarily busy, please retry",
	})
}
