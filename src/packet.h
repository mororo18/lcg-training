#ifndef __PACKET_H_
#define __PACKET_H_

#include "player_array.h"
#include "bullet_array.h"
#include "player.h"
#include "utils.h"
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

typedef
enum PacketType {
    REQUEST_PLAYER_INFO_UPDATE,
    REQUEST_BULLET_INFO_UPDATE,
    REQUEST_BULLET_SHOT,
    REQUEST_EMPTY,

    EVENT_EMPTY,
    EVENT_PLAYER_ACCEPTED,
    EVENT_PLAYER_REFUSED,
    EVENT_PLAYER_ENABLE_BOOST,
    EVENT_PLAYER_DISABLE_BOOST,
    EVENT_PLAYER_INFO_UPDATE,
    EVENT_PLAYERS_WHO_SHOT,
    EVENT_ENEMIES_POSITION_UPDATE,
    EVENT_ENEMY_KILLED,
    EVENT_BULLETS_INFO_UPDATE,
    EVENT_DESTROYED_BULLETS,
    EVENT_TYPE_RANGE
} PacketType;

static inline bool is_event(PacketType type) {
    return (type >= EVENT_EMPTY && type < EVENT_TYPE_RANGE);
}

static inline bool is_request(PacketType type) {
    return (type >= REQUEST_PLAYER_INFO_UPDATE && type <= REQUEST_EMPTY);
}

typedef
struct RequestBulletsInfoUpdate {
    BulletInfoArray info;
} RequestBulletsInfoUpdate;

typedef
struct RequestPlayerInfoUpdate {
    PlayerInfo info;
} RequestPlayerInfoUpdate;

typedef
struct DestroyedBulletsEvent {
    BulletInfoArray bullets;
} DestroyedBulletsEvent;

typedef
struct PlayersWhoShotEvent {
    PlayerIdArray who_shot;
} PlayersWhoShotEvent;

typedef 
struct EnemiesPositionUpdateEvent {
    Vector2 position[MAX_ENEMIES];
} EnemiesPositionUpdateEvent;
static_assert(sizeof(EnemiesPositionUpdateEvent) == MAX_ENEMIES * sizeof(Vector2));

typedef 
struct BulletsInfoUpdateEvent {
    BulletInfoArray info;
} BulletsInfoUpdateEvent;

typedef 
struct PlayerInfoUpdateEvent {
    int id;
    PlayerInfo info;
} PlayerInfoUpdateEvent;

typedef 
struct PlayerAcceptedEvent {
    int id;
    PlayerInfo info;
} PlayerAcceptedEvent;

typedef
union PacketData {
    RequestBulletsInfoUpdate req_bullets_info_update;
    RequestPlayerInfoUpdate req_player_info_update;

    EnemiesPositionUpdateEvent enemies_position_update;
    BulletsInfoUpdateEvent bullets_info_update_event;
    PlayerInfoUpdateEvent player_info_update_event;
    PlayerAcceptedEvent player_accepted;
    PlayersWhoShotEvent players_who_shot;
    DestroyedBulletsEvent destroyed_bullets;
} PacketData;
static_assert(sizeof(PacketData) <= 1500);

typedef
struct Packet {
    PacketType type;
    PacketData data;
} Packet;

inline static
size_t get_packet_size(PacketType type) {
    switch (type) {
        case REQUEST_BULLET_INFO_UPDATE:
            return sizeof(RequestBulletsInfoUpdate);
        case REQUEST_PLAYER_INFO_UPDATE:
            return sizeof(RequestPlayerInfoUpdate);
        case REQUEST_BULLET_SHOT:
            return 0;
        case REQUEST_EMPTY:
            return 0;

        case EVENT_PLAYER_ACCEPTED:
            return sizeof(PlayerAcceptedEvent);
        case EVENT_PLAYER_INFO_UPDATE:
            return sizeof(PlayerInfoUpdateEvent);
        case EVENT_PLAYERS_WHO_SHOT:
            return sizeof(PlayersWhoShotEvent);
        case EVENT_ENEMIES_POSITION_UPDATE:
            return sizeof(EnemiesPositionUpdateEvent);
        case EVENT_BULLETS_INFO_UPDATE:
            return sizeof(BulletsInfoUpdateEvent);
        case EVENT_DESTROYED_BULLETS:
            return sizeof(DestroyedBulletsEvent);
        case EVENT_PLAYER_ENABLE_BOOST:
            return 0;
        case EVENT_PLAYER_DISABLE_BOOST:
            return 0;
        case EVENT_PLAYER_REFUSED:
            return 0;
        case EVENT_ENEMY_KILLED:
            return 0;
        case EVENT_TYPE_RANGE:
            return 0;
        case EVENT_EMPTY:
            return 0;
        default:
            assert(false);
    }

    return 0;
}


static
const char *packet_type_to_string(PacketType type) {
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

static
void log_packet_type(const char *prefix, PacketType type) {
    printf("[%s] Packet Type: %s (%d)\n", prefix, packet_type_to_string(type), type);
}

#define PACKET_QUEUE_CAPACITY sizeof(Packet)
typedef struct {
    Packet buffer[PACKET_QUEUE_CAPACITY];
    char read_buffer[sizeof(Packet)];
    size_t read_buffer_usage;
    int client_fd;
    size_t head;
    size_t tail;
    size_t count;
} PacketQueue;

static 
bool packet_queue_enqueue(PacketQueue * packet_queue, const Packet * packet) {
    if (packet_queue->count == PACKET_QUEUE_CAPACITY) {
        return false;
    }

    packet_queue->buffer[packet_queue->tail] = *packet;
    packet_queue->tail = (packet_queue->tail + 1) % PACKET_QUEUE_CAPACITY;
    packet_queue->count++;
    return true;
}

static
bool packet_queue_dequeue(PacketQueue * packet_queue, Packet * packet) {
    if (packet_queue->count == 0) return false;  // Fila vazia
    *packet = packet_queue->buffer[packet_queue->head];
    packet_queue->head = (packet_queue->head + 1) % PACKET_QUEUE_CAPACITY;
    packet_queue->count--;
    return true;
}

static
bool packet_queue_is_empty(const PacketQueue * packet_queue) {
    return packet_queue->count == 0;
}

static
PacketQueue packet_queues[MAX_PLAYERS] = {};
static
const size_t packet_queues_lenght = sizeof(packet_queues) / sizeof(PacketQueue);

static
void init_packet_queues() {
    for (size_t i = 0; i < packet_queues_lenght; i++) {
        packet_queues[i].client_fd = -1;
        packet_queues[i].tail = 0;
        packet_queues[i].head = 0;
        packet_queues[i].count = 0;
        packet_queues[i].read_buffer_usage = 0;
    }
}

static
void assign_packet_queue_to_client(int fd) {
    assert(fd > 0);
    for (size_t i = 0; i < packet_queues_lenght; i++) {
        if (packet_queues[i].client_fd == -1) {
            packet_queues[i].client_fd = fd;
            return;
        }
    }

    assert(false);
}

static
void reset_packet_queue(int fd) {
    assert(fd > 0);
    for (size_t i = 0; i < packet_queues_lenght; i++) {
        if (packet_queues[i].client_fd == fd) {
            packet_queues[i].client_fd = -1;
            packet_queues[i].tail = 0;
            packet_queues[i].head = 0;
            packet_queues[i].count = 0;
            packet_queues[i].read_buffer_usage = 0;
            return;
        }
    }

    assert(false);
}

static
PacketQueue * get_packet_queue(int fd) {
    assert(fd > 0);
    for (size_t i = 0; i < packet_queues_lenght; i++) {
        if (packet_queues[i].client_fd == fd) {
            return packet_queues + i;
        }
    }

    assert(false);
}

static
void assert_buffer_fitness(char * ptr, size_t size, char * buff, size_t buff_size) {
    char * begin = buff;
    char * end = buff + buff_size;
    bool condition = begin <= ptr && ptr + size <= end;
    assert(condition);
}

static
void write_to_buffer(char **ptr, const void *data, size_t size) {
    memcpy(*ptr, data, size);
    *ptr += size;
}

static
void read_from_buffer(char **ptr, void *data, size_t size) {
    memcpy(data, *ptr, size);
    *ptr += size;
}

static
void write_array_to_buffer(char **ptr, const void *array, size_t element_size, size_t lenght) {
    write_to_buffer(ptr, &lenght, sizeof(size_t));
    write_to_buffer(ptr, array, element_size * lenght);
}

static
void read_array_from_buffer(char **ptr, void *array, size_t element_size, size_t *lenght) {
    read_from_buffer(ptr, lenght, sizeof(size_t));
    read_from_buffer(ptr, array, element_size * (*lenght));
}

static
size_t write_packet_to_buffer(const Packet * packet, char *buff, size_t buff_size) {
    assert(buff_size >= sizeof(Packet));
    char * ptr = buff;
    write_to_buffer(&ptr, &packet->type, sizeof(PacketType));

    switch (packet->type) {
        case REQUEST_BULLET_INFO_UPDATE:
            {
                const BulletInfo * array_ptr = packet->data.req_bullets_info_update.info.data;
                size_t array_lenght = packet->data.req_bullets_info_update.info.lenght;

                write_array_to_buffer(
                    &ptr,
                    array_ptr,
                    sizeof(BulletInfo),
                    array_lenght
                );
            }
            break;
        case REQUEST_PLAYER_INFO_UPDATE:
            {
                write_to_buffer(&ptr, &packet->data.req_player_info_update, sizeof(RequestPlayerInfoUpdate));
            }
            break;
        case REQUEST_BULLET_SHOT:
            break;
        case REQUEST_EMPTY:
            break;
        case EVENT_PLAYER_ACCEPTED:
            {
                write_to_buffer(&ptr, &packet->data.player_accepted, sizeof(PlayerAcceptedEvent));
            }
            break;
        case EVENT_PLAYER_INFO_UPDATE:
            {
                write_to_buffer(&ptr, &packet->data.player_info_update_event, sizeof(PlayerInfoUpdateEvent));
            }
            break;
        case EVENT_PLAYERS_WHO_SHOT:
            {
                const PlayerId * array_ptr = packet->data.players_who_shot.who_shot.data;
                const size_t array_lenght = packet->data.players_who_shot.who_shot.lenght;

                write_array_to_buffer(
                    &ptr,
                    array_ptr,
                    sizeof(PlayerId),
                    array_lenght
                );
            }
            break;
        case EVENT_ENEMIES_POSITION_UPDATE:
            {
                write_to_buffer(&ptr, &packet->data.enemies_position_update, sizeof(EnemiesPositionUpdateEvent));
            }
            break;
        case EVENT_BULLETS_INFO_UPDATE:
            {
                const BulletInfo * array_ptr = packet->data.bullets_info_update_event.info.data;
                const size_t array_lenght = packet->data.bullets_info_update_event.info.lenght;

                write_array_to_buffer(
                    &ptr,
                    array_ptr,
                    sizeof(BulletInfo),
                    array_lenght
                );
            }
            break;
        case EVENT_DESTROYED_BULLETS:
            {
                const BulletInfo * array_ptr = packet->data.destroyed_bullets.bullets.data;
                const size_t array_lenght = packet->data.destroyed_bullets.bullets.lenght;

                write_array_to_buffer(
                    &ptr,
                    array_ptr,
                    sizeof(BulletInfo),
                    array_lenght
                );
            }
            break;
        case EVENT_PLAYER_REFUSED:
        case EVENT_PLAYER_ENABLE_BOOST:
        case EVENT_PLAYER_DISABLE_BOOST:
        case EVENT_ENEMY_KILLED:
        case EVENT_TYPE_RANGE:
        case EVENT_EMPTY:
            break;
    }

    assert((size_t)(ptr - buff) <= sizeof(Packet));
    return (size_t)(ptr - buff);
}

static
size_t read_packet_from_buffer(Packet * packet, char * buff, size_t buff_size, size_t offset) {
    char * const src = buff + offset;
    char * ptr = src;

    // Check if there is enough data to read the EvenType
    if (buff_size - offset < sizeof(PacketType)) {
        return 0;
    }

    PacketType packet_type = *((PacketType*) ptr);
    size_t packet_size = get_packet_size(packet_type);

    // Check if there is enough data to read the PacketData
    if (buff_size - offset < sizeof(PacketType) + packet_size) {
        return 0;
    }

    read_from_buffer(&ptr, &packet->type, sizeof(PacketType));

    //printf("Decod packet type %s\n", packet_type_to_string(packet->type));

    switch (packet->type) {
        case REQUEST_BULLET_INFO_UPDATE:
            {
                BulletInfo * array_ptr = packet->data.req_bullets_info_update.info.data;
                size_t * array_lenght = &packet->data.req_bullets_info_update.info.lenght;

                read_array_from_buffer(
                    &ptr,
                    array_ptr,
                    sizeof(BulletInfo),
                    array_lenght
                );
            }
            break;
        case REQUEST_PLAYER_INFO_UPDATE:
            {
                read_from_buffer(&ptr, &packet->data.req_player_info_update, sizeof(RequestPlayerInfoUpdate));
            }
            break;
        case REQUEST_BULLET_SHOT:
            break;
        case REQUEST_EMPTY:
            break;

        case EVENT_PLAYER_ACCEPTED:
            {
                read_from_buffer(&ptr, &packet->data.player_accepted, sizeof(PlayerAcceptedEvent));
            }
            break;
        case EVENT_PLAYER_INFO_UPDATE:
            {
                read_from_buffer(&ptr, &packet->data.player_info_update_event, sizeof(PlayerInfoUpdateEvent));
            }
            break;
        case EVENT_PLAYERS_WHO_SHOT:
            {
                PlayerId * array_ptr = packet->data.players_who_shot.who_shot.data;
                size_t * array_lenght = &packet->data.players_who_shot.who_shot.lenght;

                read_array_from_buffer(
                    &ptr,
                    array_ptr,
                    sizeof(PlayerId),
                    array_lenght
                );
            }
            break;
        case EVENT_ENEMIES_POSITION_UPDATE:
            {
                read_from_buffer(&ptr, &packet->data.enemies_position_update, sizeof(EnemiesPositionUpdateEvent));
            }
            break;
        case EVENT_BULLETS_INFO_UPDATE:
            {
                BulletInfo * array_ptr = packet->data.bullets_info_update_event.info.data;
                size_t * array_lenght = &packet->data.bullets_info_update_event.info.lenght;

                read_array_from_buffer(
                    &ptr,
                    array_ptr,
                    sizeof(BulletInfo),
                    array_lenght
                );
            }
            break;
        case EVENT_DESTROYED_BULLETS:
            {
                BulletInfo * array_ptr = packet->data.destroyed_bullets.bullets.data;
                size_t * array_lenght = &packet->data.destroyed_bullets.bullets.lenght;

                read_array_from_buffer(
                    &ptr,
                    array_ptr,
                    sizeof(BulletInfo),
                    array_lenght
                );
            } break;
        case EVENT_PLAYER_REFUSED:
        case EVENT_PLAYER_ENABLE_BOOST:
        case EVENT_PLAYER_DISABLE_BOOST:
        case EVENT_ENEMY_KILLED:
        case EVENT_EMPTY:
            break;
    }

    assert((uint64_t)(ptr - src) <= sizeof(Packet));
    return (size_t)(ptr - src);
}

static
void send_packet(const Packet * packet, int fd) {
    char buff[sizeof(Packet)] = {};
    size_t bytes_count = write_packet_to_buffer(packet, (char*)&buff, sizeof(buff));
    assert(bytes_count <= sizeof(buff));

    char * buff_ptr = buff;
    while (bytes_count > 0) {
        size_t write_bytes = bytes_count;
        if (bytes_count > MTU_SIZE) {
            write_bytes = MTU_SIZE;
        }

        //ssize_t ret = send(fd, buff_ptr, write_bytes, MSG_DONTWAIT);
        ssize_t ret = write(fd, buff_ptr, write_bytes);

        if (ret == -1) {
            // TODO: treat errno
            assert(false);
        }

        bytes_count -= (size_t) ret;
        buff_ptr += ret;
    }
}

static
bool pending_packets(int fd, PacketQueue * pqueue, bool fd_ready_for_reading) {

    if (packet_queue_is_empty(pqueue) && fd_ready_for_reading) {

        ssize_t ret = 0;
        while (true) {
            ret = read(
                fd,
                pqueue->read_buffer + pqueue->read_buffer_usage,
                sizeof(pqueue->read_buffer) - pqueue->read_buffer_usage
            );

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
            Packet empty_packet = {
                .type = EVENT_EMPTY,
            };
            // enqueue
            bool enqueued = packet_queue_enqueue(pqueue, &empty_packet);
            assert(enqueued);
        }

        //printf("Packet: Recebidos %ld bytes\n", ret);

        if (ret != -1) {
            pqueue->read_buffer_usage += (size_t) ret;
        } else {
            assert(false);
        }

        while (pqueue->read_buffer_usage > 0) {
            Packet new_packet = {};
            size_t bytes_read = read_packet_from_buffer(
                &new_packet,
                (char*)&pqueue->read_buffer,
                pqueue->read_buffer_usage,
                0
            );

            //printf("Decod %ld bytes\n", bytes_read);
            if (bytes_read > 0) {
                //log_packet_received(&new_packet);
                bool enqueued = packet_queue_enqueue(pqueue, &new_packet);
                assert(enqueued);
                //log_packet_type("SENDING", new_packet.type);

                assert(pqueue->read_buffer_usage >= bytes_read);
                pqueue->read_buffer_usage -= bytes_read;
                memmove(
                    pqueue->read_buffer,
                    pqueue->read_buffer + bytes_read,
                    pqueue->read_buffer_usage
                );
            }

            if (bytes_read == 0) {
                break;
            }
        }
    }

    //bool dequeued = packet_queue_dequeue(packet);
    //assert(dequeued);
    return !packet_queue_is_empty(pqueue);
}

static
void recieve_packet(Packet * packet, PacketQueue * pqueue) {
    bool dequeued = packet_queue_dequeue(pqueue, packet);
    //log_packet_type("RECIEVED", packet->type);
    assert(dequeued);
}

#endif // __PACKET_H_
