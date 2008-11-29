#ifndef TAPCFG_H
#define TAPCFG_H

/* Define syslog style log levels */
#define TAPLOG_EMERG       0       /* system is unusable */
#define TAPLOG_ALERT       1       /* action must be taken immediately */
#define TAPLOG_CRIT        2       /* critical conditions */
#define TAPLOG_ERR         3       /* error conditions */
#define TAPLOG_WARNING     4       /* warning conditions */
#define TAPLOG_NOTICE      5       /* normal but significant condition */
#define TAPLOG_INFO        6       /* informational */
#define TAPLOG_DEBUG       7       /* debug-level messages */

/**
 * Set the current logging level, all messages with lower
 * significance than the set level will be ignored.
 * @param level is the new level of logging
 */
void taplog_set_level(int level);

/**
 * Typedef to the structure used by the library, should never
 * be accessed directly.
 */
typedef struct tapcfg_s tapcfg_t;

/**
 * Initializes a new tapcfg_t structure and allocates
 * the required memory for it.
 * @return A pointer to the tapcfg_t structure to be used
 */
tapcfg_t *tapcfg_init();

/**
 * Destroys a tapcfg_t structure and frees all resources
 * related to it cleanly. Will also stop the device in
 * case it is started. The handle can't be used any more
 * after calling this function.
 * @param tapcfg is a pointer to an inited structure
 */
void tapcfg_destroy(tapcfg_t *tapcfg);


/**
 * Creates a new network interface with the specified name
 * and waits for subsequent calls. This has to be called
 * before any calls to other functions except init, or
 * the other functions will simply fail. If the specified
 * interface name is not available, will fallback to using
 * the system default.
 * @param tapcfg is a pointer to an inited structure
 * @param ifname is a pointer to the suggested name for 
 *        the device in question in UTF-8 encoding, can be
 *        null for system default
 * @return Negative value on error, non-negative on success.
 */
int tapcfg_start(tapcfg_t *tapcfg, const char *ifname);

/**
 * Stops the network interface and frees all resources
 * related to it. After this a new interface using the
 * tcptap_start can be started for the same structure.
 * @param tapcfg is a pointer to an inited structure
 */
void tapcfg_stop(tapcfg_t *tapcfg);


/**
 * Wait for data to be available for reading. This can
 * be used for avoiding blocking the thread on read. If
 * a positive value is returned, then the following read
 * will not block in any case.
 * @param tapcfg is a pointer to an inited structure
 * @param msec is the time in milliseconds to wait for the
 *        device to become readable, can be 0 in which case
 *        the function will return immediately
 * @return Non-zero if the device is readable, zero otherwise.
 */
int tapcfg_wait_readable(tapcfg_t *tapcfg, int msec);

/**
 * Read data from the device. This function will always
 * block until there is data readable. The blocking can be
 * avoided by making sure there is data available by using
 * the tapcfg_wait_readable function. The buffer should
 * always have enough space for a complete Ethernet frame
 * or the read will simply fail.
 * @param tapcfg is a pointer to an inited structure
 * @param buf is a pointer to the buffer where data is read to
 * @param count is the maximum size of the buffer
 * @return Negative value on error, number of bytes read otherwise.
 */
int tapcfg_read(tapcfg_t *tapcfg, void *buf, int count);

/**
 * Wait for data to be available for writing. This can
 * be used for avoiding blocking the thread on write. If
 * a positive value is returned, then the following write
 * will not block in any case.
 * @param tapcfg is a pointer to an inited structure
 * @param msec is the time in milliseconds to wait for the
 *        device to become writable, can be 0 in which case
 *        the function will return immediately
 * @return Non-zero if the device is writable, zero otherwise.
 */
int tapcfg_wait_writable(tapcfg_t *tapcfg, int msec);

/**
 * Write data to the device. This function will always
 * block until the device is writable. The blocking can be
 * avoided by making sure the device is writable by using
 * the tapcfg_wait_readable function. However in common case
 * write should be very fast and that is not necessary.
 * The buffer should always contain a complete Ethernet frame
 * to write.
 * @param tapcfg is a pointer to an inited structure
 * @param buf is a pointer to the buffer where data is written from
 * @param count is the number of bytes in the buffer
 * @return Negative value on error, number of bytes written otherwise.
 */
int tapcfg_write(tapcfg_t *tapcfg, void *buf, int count);


/**
 * Get the current name of the interface. This can be falled
 * after tapcfg_start to see if the suggested interface name
 * was available for use.
 * @param tapcfg is a pointer to an inited structure
 * @return Pointer to a character array containing the interface
 *         name in UTF-8 encoding, should not be freed by the
 *         caller application! NULL if the device is not started.
 */
const char *tapcfg_get_ifname(tapcfg_t *tapcfg);


/**
 * Get the status of the interface. In Unix systems this means
 * if the interface is up or down and in Windows it means if the
 * network cable of the virtual interface is connected or not.
 * @param tapcfg is a pointer to an inited structure
 * @return Non-zero if the interface is enabled, zero otherwise.
 */
int tapcfg_iface_get_status(tapcfg_t *tapcfg);

/**
 * Get the status of the interface. In Unix systems this means
 * if the interface is up or down and in Windows it means if the
 * network cable of the virtual interface is connected or not.
 * @param tapcfg is a pointer to an inited structure
 * @param enabled is the new status of the interface after call
 * @return Negative value if an error happened, non-negative otherwise.
 */
int tapcfg_iface_change_status(tapcfg_t *tapcfg, int enabled);

/**
 * Set the maximum transfer unit for the device if possible, this function
 * will fail on some systems like Windows 2000 or Windows XP and that is ok.
 * The IP stack on those platforms doesn't support dynamic MTU and it should
 * not cause trouble in any other functionality.
 * @param tapcfg is a pointer to an inited structure
 * @param mtu is the new maximum transfer unit after calling the function
 */
int tapcfg_iface_set_mtu(tapcfg_t *tapcfg, int mtu);

/**
 * Set the IPv4 address and netmask of the interface and update
 * the routing tables accordingly.
 * @param tapcfg is a pointer to an inited structure
 * @param addr is a string containing the address in standard numeric format
 * @param netbits is the number of bits in the netmask for this subnet,
 *        must be between [1, 32]
 */
int tapcfg_iface_set_ipv4(tapcfg_t *tapcfg, const char *addr, unsigned char netbits);

/**
 * Set the IPv6 address and netmask of the interface and update
 * the routing tables accordingly.
 * @param tapcfg is a pointer to an inited structure
 * @param addr is a string containing the address in standard numeric format
 * @param netbits is the number of bits in the netmask for this subnet,
 *        must be between [1, 128]
 */
int tapcfg_iface_add_ipv6(tapcfg_t *tapcfg, const char *addr, unsigned char netbits);

#endif /* TAPCFG_H */

