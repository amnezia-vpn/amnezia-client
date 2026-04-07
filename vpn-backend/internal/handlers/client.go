package handlers

import (
	"net/http"

	"vpn-backend/internal/config"

	"github.com/gin-gonic/gin"
)

// ClientVersionResponse описывает данные об обновлениях клиента
type ClientVersionResponse struct {
	Version      string `json:"version"`
	DownloadURL  string `json:"download_url"`
	ReleaseNotes string `json:"release_notes"`
	IsCritical   bool   `json:"is_critical"`
}

// GetLatestClientVersion возвращает актуальную информацию о версии клиента
func GetLatestClientVersion(cfg *config.Config) gin.HandlerFunc {
	return func(c *gin.Context) {
		// В будущем можно добавить проверку OS (c.Query("os")),
		// но сейчас отдаём универсальный ответ из конфига.
		resp := ClientVersionResponse{
			Version:      cfg.ClientLatestVersion,
			DownloadURL:  cfg.ClientDownloadURL,
			ReleaseNotes: cfg.ClientReleaseNotes,
			IsCritical:   cfg.ClientUpdateCritical,
		}

		c.JSON(http.StatusOK, resp)
	}
}
