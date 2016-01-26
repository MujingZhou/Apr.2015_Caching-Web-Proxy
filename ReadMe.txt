This is the ReadMe file of Mujing's Caching Web Proxy written in C. This is a course project for 15-213 (Intro to Computer Systems).

In this project, I implemented an HTTP web proxy efficiently handle multiple concurrent requests.


Detailed introductions:
/* 
 * Simple HTTP proxy that caches web objects.
 * 
 *
 * Name :     Mujing Zhou
 * Andrew ID: mujingz
 *
 *
 *
 * Client: Web browser
 * Proxy : This program run on shark machine
 * Server: Web server you visit from browser
 *
 *
 * Step 1:
 * Simple proxy based on TINY. Read and parse requests from web browser
 * and reorganize the headers that are sended to server. Read and directly
 * forward contents from server to web browser.
 *
 * Step 2:
 * Proxy based on Step 1 and concurency is added. Multiple concurrent connections
 * are dealt with.
 *
 * Step 3:
 * Proxy based on Step 2 and cache is added to cache web contents. LRU caching
 * mechanism is used for eviction blocks from cache. Race conditions considered.
 *
 */
