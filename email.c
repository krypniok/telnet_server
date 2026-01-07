#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "email.h"

// Struktur, um Daten an den Worker-Thread zu übergeben
typedef struct {
    char *to;
    char *subject;
    char *body;
} EmailJob;

// Der Worker-Thread: Ruft msmtp blockierend auf (läuft aber im Hintergrund)
static void *email_worker(void *arg) {
    EmailJob *job = (EmailJob *)arg;
    if (!job) return NULL;

    // msmtp mit -t aufrufen (liest Empfänger und Infos aus dem Header)
    FILE *fp = popen("msmtp -t", "w");
    if (fp) {
        // E-Mail-Header schreiben
        fprintf(fp, "To: %s\n", job->to);
        fprintf(fp, "Subject: %s\n", job->subject);
        fprintf(fp, "Content-Type: text/plain; charset=utf-8\n");
        // Optional: From setzen, falls nicht in .msmtprc definiert
        // fprintf(fp, "From: noreply@dein-server.de\n");
        fprintf(fp, "\n"); // Leerzeile trennt Header vom Body
        
        // Body schreiben
        fprintf(fp, "%s\n", job->body);
        
        // Pipe schließen (sendet EOF an msmtp und löst Versand aus)
        pclose(fp);
    } else {
        perror("Fehler beim Starten von msmtp");
    }

    // Speicher aufräumen
    free(job->to);
    free(job->subject);
    free(job->body);
    free(job);

    return NULL;
}

// Die öffentliche Funktion: Startet den Thread und kehrt sofort zurück
void send_email_async(const char *to, const char *subject, const char *body) {
    if (!to || !subject || !body) return;

    EmailJob *job = malloc(sizeof(EmailJob));
    if (!job) return;

    // Strings kopieren, da die Originale ungültig werden könnten
    job->to = strdup(to);
    job->subject = strdup(subject);
    job->body = strdup(body);

    pthread_t tid;
    if (pthread_create(&tid, NULL, email_worker, job) == 0) {
        pthread_detach(tid); // Thread läuft selbstständig, kein join nötig
    } else {
        // Fallback: Speicher freigeben, wenn Thread nicht startet
        free(job->to);
        free(job->subject);
        free(job->body);
        free(job);
    }
}