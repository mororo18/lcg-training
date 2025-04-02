#ifndef __PACKET_H_
#define __PACKET_H_

#include "player_array.h"
#include "bullet_array.h"
#include "player.h"
#include "config.h"
#include "player_array.h"
#include "bullet_array.h"
#include <errno.h>
#include <string.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>

#define MTU_SIZE 1500

typedef enum  __attribute__ ((__packed__)) PacketType { REQUEST_PLAYER_INFO_UPDATE, REQUEST_BULLET_INFO_UPDATE, REQUEST_BULLET_SHOT, REQUEST_EMPTY, EVENT_EMPTY, EVENT_PLAYER_ACCEPTED, EVENT_PLAYER_REFUSED, EVENT_PLAYER_ENABLE_BOOST, EVENT_PLAYER_DISABLE_BOOST, EVENT_PLAYER_INFO_UPDATE, EVENT_PLAYERS_WHO_SHOT, EVENT_ENEMIES_POSITION_UPDATE, EVENT_ENEMY_KILLED, EVENT_BULLETS_INFO_UPDATE, EVENT_DESTROYED_BULLETS, EVENT_TYPE_RANGE, PACKET_INVALID, } PacketType;

static inline bool is_event(PacketType type) { return (type >= EVENT_EMPTY && type < EVENT_TYPE_RANGE); }

static inline bool is_request(PacketType type) { return (type >= REQUEST_PLAYER_INFO_UPDATE && type <= REQUEST_EMPTY); }

typedef struct RequestBulletsInfoUpdate { BulletInfoArray info; }
RequestBulletsInfoUpdate;

typedef struct RequestPlayerInfoUpdate { PlayerInfo info; }
RequestPlayerInfoUpdate;

typedef struct DestroyedBulletsEvent { BulletInfoArray bullets; }
DestroyedBulletsEvent;

typedef struct PlayersWhoShotEvent { PlayerIdArray who_shot; }
PlayersWhoShotEvent;

typedef struct EnemiesPositionUpdateEvent { Vector2 position[MAX_ENEMIES]; }
EnemiesPositionUpdateEvent;

typedef struct BulletsInfoUpdateEvent { BulletInfoArray info; }
BulletsInfoUpdateEvent;

typedef struct PlayerInfoUpdateEvent {
    int id;
    PlayerInfo info;
} PlayerInfoUpdateEvent;

typedef struct PlayerAcceptedEvent {
    int id;
    PlayerInfo info;
} PlayerAcceptedEvent;

typedef union PacketData {
    RequestBulletsInfoUpdate req_bullets_info_update;
    RequestPlayerInfoUpdate req_player_info_update;

    EnemiesPositionUpdateEvent enemies_position_update;
    BulletsInfoUpdateEvent bullets_info_update_event;
    PlayerInfoUpdateEvent player_info_update_event;
    PlayerAcceptedEvent player_accepted;
    PlayersWhoShotEvent players_who_shot;
    DestroyedBulletsEvent destroyed_bullets;
} PacketData;

typedef struct Packet {
    PacketType type;
    PacketData data;
} Packet;

#define PACKET_SIZE sizeof(Packet)

inline static size_t get_packet_data_size(PacketType type) {
    switch (type) {
        case REQUEST_BULLET_INFO_UPDATE: return sizeof(RequestBulletsInfoUpdate);
        case REQUEST_PLAYER_INFO_UPDATE: return sizeof(RequestPlayerInfoUpdate);
        case REQUEST_BULLET_SHOT: return 0;
        case REQUEST_EMPTY: return 0;

        case EVENT_PLAYER_ACCEPTED: return sizeof(PlayerAcceptedEvent);
        case EVENT_PLAYER_INFO_UPDATE: return sizeof(PlayerInfoUpdateEvent);
        case EVENT_PLAYERS_WHO_SHOT: return sizeof(PlayersWhoShotEvent);
        case EVENT_ENEMIES_POSITION_UPDATE: return sizeof(EnemiesPositionUpdateEvent);
        case EVENT_BULLETS_INFO_UPDATE: return sizeof(BulletsInfoUpdateEvent);
        case EVENT_DESTROYED_BULLETS: return sizeof(DestroyedBulletsEvent);
        case EVENT_PLAYER_ENABLE_BOOST: return 0;
        case EVENT_PLAYER_DISABLE_BOOST: return 0;
        case EVENT_PLAYER_REFUSED: return 0;
        case EVENT_ENEMY_KILLED: return 0;
        case EVENT_TYPE_RANGE: return 0;
        case EVENT_EMPTY: return 0;
        default: assert(false);
    }

    return 0;
}

static bool packet_data_is_array(PacketType type) {
    switch (type) {
        case REQUEST_BULLET_INFO_UPDATE:
        case EVENT_PLAYERS_WHO_SHOT:
        case EVENT_BULLETS_INFO_UPDATE:
        case EVENT_DESTROYED_BULLETS: return true;
        default: return false;
    }
}

static const char *packet_type_to_string(PacketType type) {
    switch (type) {
        case REQUEST_PLAYER_INFO_UPDATE: return "REQUEST_PLAYER_INFO_UPDATE";
        case REQUEST_BULLET_INFO_UPDATE: return "REQUEST_BULLET_INFO_UPDATE";
        case REQUEST_BULLET_SHOT: return "REQUEST_BULLET_SHOT";
        case REQUEST_EMPTY: return "REQUEST_EMPTY";
        case EVENT_EMPTY: return "EVENT_EMPTY";
        case EVENT_PLAYER_ACCEPTED: return "EVENT_PLAYER_ACCEPTED";
        case EVENT_PLAYER_REFUSED: return "EVENT_PLAYER_REFUSED";
        case EVENT_PLAYER_ENABLE_BOOST: return "EVENT_PLAYER_ENABLE_BOOST";
        case EVENT_PLAYER_DISABLE_BOOST: return "EVENT_PLAYER_DISABLE_BOOST";
        case EVENT_PLAYER_INFO_UPDATE: return "EVENT_PLAYER_INFO_UPDATE";
        case EVENT_PLAYERS_WHO_SHOT: return "EVENT_PLAYERS_WHO_SHOT";
        case EVENT_ENEMIES_POSITION_UPDATE: return "EVENT_ENEMIES_POSITION_UPDATE";
        case EVENT_ENEMY_KILLED: return "EVENT_ENEMY_KILLED";
        case EVENT_BULLETS_INFO_UPDATE: return "EVENT_BULLETS_INFO_UPDATE";
        case EVENT_DESTROYED_BULLETS: return "EVENT_DESTROYED_BULLETS";
        case EVENT_TYPE_RANGE: return "EVENT_TYPE_RANGE";
        default: return "UNKNOWN_PACKET_TYPE";
    }
}

/*
static inline void log_packet_type(const char *prefix, PacketType type) {
    printf("[%s] Packet Type: %s (%d)\n", prefix, packet_type_to_string(type), type);
}
*/

#define PACKET_QUEUE_CAPACITY PACKET_SIZE
typedef struct {
    Packet buffer[PACKET_QUEUE_CAPACITY];
    char read_buffer[PACKET_SIZE];
    size_t read_buffer_usage;
    int client_fd;
    size_t head;
    size_t tail;
    size_t count;
} PacketQueue;

static Packet * packet_queue_enqueue(PacketQueue * packet_queue) {
    if (packet_queue->count == PACKET_QUEUE_CAPACITY) { assert(false); }
    Packet * ret = packet_queue->buffer + packet_queue->tail;
    packet_queue->tail = (packet_queue->tail + 1) % PACKET_QUEUE_CAPACITY;
    packet_queue->count++;
    return ret;
}

static Packet * packet_queue_dequeue(PacketQueue * packet_queue) {
    if (packet_queue->count == 0) { assert(false); }  // Fila vazia
    Packet * ret = packet_queue->buffer + packet_queue->head;
    packet_queue->head = (packet_queue->head + 1) % PACKET_QUEUE_CAPACITY;
    packet_queue->count--;
    return ret;
}

static Packet * packet_queue_pop(PacketQueue *packet_queue) {
    if (packet_queue->count == 0) { return NULL; }

    packet_queue->tail = (packet_queue->tail - 1 + PACKET_QUEUE_CAPACITY) % PACKET_QUEUE_CAPACITY;
    packet_queue->count--;

    return packet_queue->buffer + packet_queue->tail;
}

static PacketQueue packet_queues[MAX_PLAYERS] = {};
static const size_t packet_queues_lenght = sizeof(packet_queues) / sizeof(PacketQueue);

static inline void init_packet_queues() {
    for (size_t i = 0; i < packet_queues_lenght; i++) { packet_queues[i] = (PacketQueue) { .client_fd = -1 }; }
}

static inline void assign_packet_queue_to_client(int fd) {
    assert(fd > 0);
    for (size_t i = 0; i < packet_queues_lenght; i++) {
        if (packet_queues[i].client_fd == -1) {
            packet_queues[i].client_fd = fd;
            return;
        }
    }

    assert(false);
}

static inline void reset_packet_queue(int fd) {
    assert(fd > 0);
    for (size_t i = 0; i < packet_queues_lenght; i++) if (packet_queues[i].client_fd == fd) { packet_queues[i] = (PacketQueue) { .client_fd = -1 }; }
}

static inline PacketQueue * get_packet_queue(int fd) {
    assert(fd > 0);
    for (size_t i = 0; i < packet_queues_lenght; i++) if (packet_queues[i].client_fd == fd) return packet_queues + i;
    assert(false);
}

/*
static inline void assert_buffer_fitness(char * ptr, size_t size, char * buff, size_t buff_size) {
    assert(buff <= ptr && ptr + size <= buff + buff_size);
}
*/

static void write_to_buffer(char **ptr, const void *data, size_t size) {
    memcpy(*ptr, data, size);
    *ptr += size;
}

static void read_from_buffer(char **ptr, void *data, size_t size) {
    memcpy(data, *ptr, size);
    *ptr += size;
}

static void write_array_to_buffer(char **ptr, const void *array, size_t element_size, size_t lenght) {
    write_to_buffer(ptr, &lenght, sizeof(size_t));
    write_to_buffer(ptr, array, element_size * lenght);
}

static void read_array_from_buffer(char **ptr, void *array, size_t element_size, size_t *lenght) {
    read_from_buffer(ptr, lenght, sizeof(size_t));
    read_from_buffer(ptr, array, element_size * (*lenght));
}

static size_t write_packet_to_buffer(const Packet * packet, char *buff, size_t buff_size) {
    assert(buff_size >= PACKET_SIZE);
    char * ptr = buff;
    write_to_buffer(&ptr, &packet->type, sizeof(PacketType));

    switch (packet->type) {
        case REQUEST_BULLET_INFO_UPDATE: write_array_to_buffer( &ptr, packet->data.req_bullets_info_update.info.data, sizeof(BulletInfo), packet->data.req_bullets_info_update.info.lenght);
            break;
        case REQUEST_PLAYER_INFO_UPDATE: write_to_buffer(&ptr, &packet->data.req_player_info_update, sizeof(RequestPlayerInfoUpdate));
            break;
        case EVENT_PLAYER_ACCEPTED: write_to_buffer(&ptr, &packet->data.player_accepted, sizeof(PlayerAcceptedEvent));
            break;
        case EVENT_PLAYER_INFO_UPDATE: write_to_buffer(&ptr, &packet->data.player_info_update_event, sizeof(PlayerInfoUpdateEvent));
            break;
        case EVENT_PLAYERS_WHO_SHOT: write_array_to_buffer( &ptr, packet->data.players_who_shot.who_shot.data, sizeof(PlayerId), packet->data.players_who_shot.who_shot.lenght);
            break;
        case EVENT_ENEMIES_POSITION_UPDATE: write_to_buffer(&ptr, &packet->data.enemies_position_update, sizeof(EnemiesPositionUpdateEvent));
            break;
        case EVENT_BULLETS_INFO_UPDATE: write_array_to_buffer( &ptr, packet->data.bullets_info_update_event.info.data, sizeof(BulletInfo), packet->data.bullets_info_update_event.info.lenght);
            break;
        case EVENT_DESTROYED_BULLETS: write_array_to_buffer( &ptr, packet->data.destroyed_bullets.bullets.data, sizeof(BulletInfo), packet->data.destroyed_bullets.bullets.lenght);
            break;
        default:
            break;
    }

    assert((size_t)(ptr - buff) <= PACKET_SIZE);
    return (size_t)(ptr - buff);
}

static size_t read_packet_from_buffer(Packet * packet, char * buff, size_t buff_size) {
    char * const src = buff;
    char * ptr = src;

    // Check if there is enough data to read the EvenType
    if (buff_size < sizeof(PacketType)) return 0;

    PacketType packet_type = *((PacketType*) ptr);

    if (packet_data_is_array(packet_type)) {

        // Verifica se há espaço para ler o tamanho do array
        if ((size_t)(ptr - buff) + sizeof(size_t) > buff_size) return 0;

        read_from_buffer(&ptr, &packet->type, sizeof(PacketType));

        size_t element_size = 0;
        void * array_ptr = NULL;
        size_t * array_lenght_ptr = NULL;

        // TODO: minimize this
        switch (packet_type) {
            case REQUEST_BULLET_INFO_UPDATE: {
                    array_ptr = packet->data.req_bullets_info_update.info.data;
                    array_lenght_ptr = &packet->data.req_bullets_info_update.info.lenght;
                    element_size = sizeof(BulletInfo);
                } break;
            case EVENT_PLAYERS_WHO_SHOT: {
                    array_ptr = packet->data.players_who_shot.who_shot.data;
                    array_lenght_ptr = &packet->data.players_who_shot.who_shot.lenght;
                    element_size = sizeof(PlayerId);
                } break;
            case EVENT_BULLETS_INFO_UPDATE: {
                    array_ptr = packet->data.bullets_info_update_event.info.data;
                    array_lenght_ptr = &packet->data.bullets_info_update_event.info.lenght;
                    element_size = sizeof(BulletInfo);
                } break;
            case EVENT_DESTROYED_BULLETS: {
                    array_ptr = packet->data.destroyed_bullets.bullets.data;
                    array_lenght_ptr = &packet->data.destroyed_bullets.bullets.lenght;
                    element_size = sizeof(BulletInfo);
                } break;
            default: assert(false);
        }

        size_t actual_array_len = *((size_t*) ptr);

        // Verifica se há espaço suficiente para os elementos do array
        if ((size_t)(ptr - buff) + (actual_array_len * element_size) > buff_size) return 0;

        read_array_from_buffer( &ptr, array_ptr, element_size, array_lenght_ptr);

    } else {
        size_t packet_size = get_packet_data_size(packet_type);
        // Check if there is enough data to read the PacketData
        if (buff_size < sizeof(PacketType) + packet_size) { return 0; }

        read_from_buffer(&ptr, &packet->type, sizeof(PacketType));

        switch (packet->type) {
            case REQUEST_PLAYER_INFO_UPDATE: read_from_buffer(&ptr, &packet->data.req_player_info_update, sizeof(RequestPlayerInfoUpdate));
                break;
            case EVENT_PLAYER_ACCEPTED: read_from_buffer(&ptr, &packet->data.player_accepted, sizeof(PlayerAcceptedEvent));
                break;
            case EVENT_PLAYER_INFO_UPDATE: read_from_buffer(&ptr, &packet->data.player_info_update_event, sizeof(PlayerInfoUpdateEvent));
                break;
            case EVENT_ENEMIES_POSITION_UPDATE: read_from_buffer(&ptr, &packet->data.enemies_position_update, sizeof(EnemiesPositionUpdateEvent));
                break;
            default:
                break;
        }
    }

    assert((uint64_t)(ptr - src) <= PACKET_SIZE);
    return (size_t)(ptr - src);
}

static void send_packet(const Packet * packet, int fd) {
    char buff[PACKET_SIZE] = {};
    size_t bytes_count = write_packet_to_buffer(packet, (char*)&buff, sizeof(buff));
    assert(bytes_count <= sizeof(buff));

    char * buff_ptr = buff;
    while (bytes_count > 0) {
        size_t write_bytes = bytes_count;
        if (bytes_count > MTU_SIZE) write_bytes = MTU_SIZE;

        ssize_t ret = write(fd, buff_ptr, write_bytes);

        if (ret == -1) {
            switch (errno) {
                case EBADF: // Bad file descriptor
                case EPIPE: { // Broken Pipe
                        printf("Connection was closed.\n");
                        close(fd);
                        return;
                    } break;
                default: printf("Erro: %s\n", strerror(errno));
            };

            printf("erron=%u\n", errno);
        }

        bytes_count -= (size_t) ret;
        buff_ptr += ret;
    }
}

static bool pending_packets(int fd, PacketQueue * pqueue, bool fd_ready_for_reading) {

    if (pqueue->count == 0 && fd_ready_for_reading) {

        ssize_t ret = 0;
        while (true) {
            ret = read( fd, pqueue->read_buffer + pqueue->read_buffer_usage, sizeof(pqueue->read_buffer) - pqueue->read_buffer_usage);

            if (ret >= 0) { break; }


            if (errno == EINTR) {
                puts("EINTR");
                continue;  // try again
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                puts("EAGAIN || EWOULDBLOCK");
                assert(false);
            }

            // Critical error
            Packet * empty_packet = packet_queue_enqueue(pqueue);
            empty_packet->type = PACKET_INVALID;
        }

        if (ret != -1) { pqueue->read_buffer_usage += (size_t) ret; }
        else { assert(false); }

        while (pqueue->read_buffer_usage > 0) {
            Packet * packet = packet_queue_enqueue(pqueue);

            size_t bytes_read = read_packet_from_buffer( packet, (char*)&pqueue->read_buffer, pqueue->read_buffer_usage);

            if (bytes_read > 0) {

                assert(pqueue->read_buffer_usage >= bytes_read);
                pqueue->read_buffer_usage -= bytes_read;
                memmove( pqueue->read_buffer, pqueue->read_buffer + bytes_read, pqueue->read_buffer_usage);
            } else {
                // If we dont read any bytes we change back the state
                // of the queue.
                packet_queue_pop(pqueue);
                break;
            }
        }
    }

    return !(pqueue->count == 0);
}

static Packet * recieve_packet(PacketQueue * pqueue) { return packet_queue_dequeue(pqueue); }

#endif // __PACKET_H_
