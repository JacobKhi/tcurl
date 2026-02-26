#pragma once

/**
 * HTTP request timing breakdown.
 * 
 * Captures detailed timing information for each phase of an HTTP request,
 * from DNS lookup through final data transfer.
 */
typedef struct {
    double dns_ms;       /**< DNS resolution time in milliseconds */
    double tcp_ms;       /**< TCP connection time in milliseconds */
    double tls_ms;       /**< TLS handshake time in milliseconds (HTTPS only) */
    double ttfb_ms;      /**< Time to first byte in milliseconds */
    double transfer_ms;  /**< Data transfer time in milliseconds */
    double total_ms;     /**< Total request time in milliseconds */
} HttpTiming;
