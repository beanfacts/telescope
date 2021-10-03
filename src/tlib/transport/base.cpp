#include "base.hpp"

/* ---------------------- Internal Functionality ---------------------- */

void *tsc_base::alloc_aligned(size_t length, size_t *out_length)
{
    void *memptr = nullptr;
    long ps = sysconf(_SC_PAGE_SIZE);
    size_t actual = length + (ps - (length % ps));
    std::cout << "raw: " << length << ", page size: " << ps << ", alloc: "
            << actual << "\n";
    int ret = posix_memalign(&memptr, ps, actual);
    if (ret)
    {
        std::cerr << "Memory allocation error: " << strerror(errno) << "\n";
        throw std::runtime_error("Memory allocation error.");
    }
    *out_length = actual;
    return memptr;
}

void tsc_base::set_sockaddr(const char *address_str, const char *port,
        struct sockaddr_in *out_addr)
{
    int ret;
    in_addr raw_addr;

    ret = inet_aton(address_str, &raw_addr);
    if (!ret)
    {
        errno = EINVAL;
        throw std::runtime_error("Cannot convert address string to integer");
    }

    char *endptr;
    errno = 0;
    long _port = strtol(port, &endptr, 10);
    if (_port > 65535 || _port < 1)
    {
        errno = EINVAL;
        throw std::runtime_error("Invalid port provided");
    }
    else if (errno)
    {
        throw std::runtime_error(std::string("Invalid port info: ") + std::string(strerror(errno)));
    }
    
    memset(out_addr, 0, sizeof(*out_addr));
    out_addr->sin_family    = AF_INET;
    out_addr->sin_addr      = raw_addr;
    out_addr->sin_port      = htons(_port);
}

/* ---------------------- Memory Allocation Subsystem ---------------------- */

tsc_malloc::tsc_malloc(size_t length, size_t alignment)
{
    int ret;
    void *memptr;
    ret = posix_memalign(&memptr, alignment, length);
    if (ret)
    {
        std::cerr << "Memory allocation error: " << strerror(errno) << "\n";
        throw std::runtime_error("Memory allocation error.");
    }
    ret = sm_set_pool(pool, memptr, length, 0, NULL);
    if (ret)
        throw std::runtime_error("Memory pool allocation failed.");
    
    addr = memptr;
}

void tsc_malloc::set_thread_mode(bool mode)
{
    thread_mode = mode;
}

bool tsc_malloc::get_thread_mode()
{
    return thread_mode;
}

void *tsc_malloc::tmalloc(size_t size)
{
    void *memptr;
    if (thread_mode) pthread_mutex_lock(&thread_lock);
    memptr = sm_malloc_pool(pool, size);
    if (thread_mode) pthread_mutex_unlock(&thread_lock);
    return memptr;
}

void tsc_malloc::tfree(void *addr)
{
    if (thread_mode) pthread_mutex_lock(&thread_lock);
    sm_free_pool(pool, addr);
    if (thread_mode) pthread_mutex_unlock(&thread_lock);
}

/* ---------------------- Simple Message Counters ---------------------- */

inline uint64_t tsc_communicator_nb::increment_sc()
{
    send_counter++;
    if (send_counter == 0)
    {
        send_counter++;
    }
    return send_counter;
}

inline uint64_t tsc_communicator_nb::increment_rc()
{
    recv_counter++;
    if (recv_counter == 0)
    {
        recv_counter++;
    }
    return send_counter;
}