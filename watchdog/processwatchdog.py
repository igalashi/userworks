#!/usr/bin/env python3

import argparse
import json
import subprocess
import time
import redis

REDIS_HOST = "localhost"
REDIS_PORT = 6379
REDIS_DB = 1

HASH_KEY = "metrics:TimeFrameBuilder-0:LostSegments"
#SCRIPT_PATH = "./run.sh"
SCRIPT_PATH = "./reload.sh"
USE_TABLE_LOOKUP = True
CONSECUTIVE_FIELD_COUNT = 5
CONSECUTIVE_FIELD_TIMEOUT_SECONDS = 2.0
DAQCTL_CHANNEL = "daqctl"
DAQCTL_SERVICES = "all"
DAQCTL_INSTANCES = "all"
STATE_HASH_KEY = "metrics:state"
STATE_WAIT_RETRY_COUNT = 10
STATE_WAIT_SLEEP_SECONDS = 1
RECBE_SAMPLER_DB = 2
RECBE_SAMPLER_KEY_PATTERN = "parameters:RecbeSampler-*"
RECBE_SAMPLER_KEY_PREFIX = "parameters:"
RECBE_SAMPLER_IP_FIELD = "ip"
RECBE_SAMPLER_IP_PREFIX = "192.168.10."

SAMPLE_TABLE = {
    0: (0, 0),
    1: (0, 0),
    2: (0, 0),
    3: ( 10, 0x0002),  # IP 19
    4: ( 11, 0x0001),  # IP 20
    5: ( 11, 0x0002),  # IP 21
    6: ( 11, 0x0004),  # IP 22
    7: ( 12, 0x0001),  # IP 23
    8: ( 12, 0x0002),  # IP 24
    9: ( 13, 0x0001),  # IP 25
   10: ( 13, 0x0002),  # IP 26
   11: ( 14, 0x0001),  # IP 27
   12: ( 14, 0x0002),  # IP 28
   13: ( 15, 0x0001),  # IP 29
   14: ( 15, 0x0002),  # IP 30
   15: ( 15, 0x0004),  # IP 31
   16: ( 16, 0x0001),  # IP 32
   17: ( 16, 0x0002),  # IP 33
   18: ( 10, 0x0004),  # IP 34
   19: ( 10, 0x0008),  # IP 35
   20: ( 11, 0x0008),  # IP 36
   21: ( 11, 0x0010),  # IP 37
   22: ( 11, 0x0020),  # IP 38
   23: ( 12, 0x0040),  # IP 30
   24: ( 12, 0x0008),  # IP 40
   25: ( 13, 0x0004),  # IP 41
   26: ( 13, 0x0008),  # IP 42
   27: ( 14, 0x0004),  # IP 43
   28: ( 14, 0x0008),  # IP 44
   29: ( 15, 0x0008),  # IP 45
   30: ( 15, 0x0010),  # IP 46
   31: ( 15, 0x0020),  # IP 47
   32: ( 16, 0x0004),  # IP 48
   33: ( 16, 0x0208),  # IP 49
   34: ( 10, 0x0040),  # IP 50
}


def uint32_to_ipv4(value):
    # type: (str) -> str
    """
    uint32 表現の IPv4 アドレスを dotted decimal 表記へ変換する。
    """
    try:
        number = int(value, 10)
    except ValueError as exc:
        raise ValueError(f"field is not an unsigned int: {value}") from exc

    if not 0 <= number <= 0xFFFFFFFF:
        raise ValueError(f"field is outside uint32 range: {value}")

    return ".".join(
        str((number >> shift) & 0xFF)
        for shift in (24, 16, 8, 0)
    )


def lookup_integer_pair(value):
    # type: (str) -> tuple
    """
    整数値をキーにして、テーブルから二つの整数値の組を返す。
    """
    try:
        number = int(value, 10)
    except ValueError as exc:
        raise ValueError(f"field is not an integer: {value}") from exc

    try:
        return SAMPLE_TABLE[number]
    except KeyError as exc:
        raise ValueError(f"field is not in SAMPLE_TABLE: {value}") from exc


def make_field_args(field):
    # type: (str) -> list
    if USE_TABLE_LOOKUP:
        first, second = lookup_integer_pair(field)
        return [str(field), str(first), str(second)]

    return [uint32_to_ipv4(field)]


def make_script_args(hash_data):
    # type: (dict) -> list
    """
    Redis Hash を
      ip1 value1 ip2 value2 ...
    または USE_TABLE_LOOKUP=True の場合は
      integer1 integer2 value1 integer3 integer4 value2 ...
    という引数列に変換する。
    """
    args = []

    for field, value in sorted(hash_data.items()):
        args.extend(make_field_args(field))
        args.append(value)

    return args


def get_changed_fields(previous_hash, current_hash):
    # type: (dict, dict) -> list
    if previous_hash is None:
        return sorted(current_hash.items())

    changed_fields = []
    for field in sorted(set(previous_hash) | set(current_hash)):
        previous_value = previous_hash.get(field)
        current_value = current_hash.get(field)
        if previous_value != current_value:
            changed_fields.append((field, current_value))

    return changed_fields


def get_health_devices(health_redis):
    # type: (redis.Redis) -> list
    devices = []

    for key in health_redis.keys("*"):
        if "health" not in key or not key.endswith(":health"):
            continue

        dev = key[:-len(":health")]
        if dev.startswith("daq_service:"):
            dev_parts = dev.split(":", 2)
            if len(dev_parts) == 3:
                dev = dev_parts[2]

        devices.append(dev)

    return devices


def kill_fairmq_devices():
    # type: () -> None
    print("kill_fairmq_devices requested, but no Python implementation is configured.")


def state_consistency_wait(health_redis, state_redis):
    # type: (redis.Redis, redis.Redis) -> str
    devices = get_health_devices(health_redis)
    if not devices:
        print("No health devices found")
        return ""

    first_dev = devices[0]
    first_dev_state = ""

    for dev in devices:
        state = ""
        retry_index = 0

        for retry_index in range(1, STATE_WAIT_RETRY_COUNT + 1):
            first_dev_state = state_redis.hget(STATE_HASH_KEY, first_dev) or ""
            state = state_redis.hget(STATE_HASH_KEY, dev) or ""

            if state == "":
                print("Null state")
                time.sleep(STATE_WAIT_SLEEP_SECONDS)
                continue

            if state == first_dev_state:
                break

            time.sleep(STATE_WAIT_SLEEP_SECONDS)

        if retry_index == STATE_WAIT_RETRY_COUNT and state != first_dev_state:
            kill_fairmq_devices()
            break

    return first_dev_state


def state_wait(health_redis, state_redis, req_state):
    # type: (redis.Redis, redis.Redis, str) -> None
    consistent_state = state_consistency_wait(health_redis, state_redis)
    if req_state != consistent_state:
        print("states are not consistent!!!")


def status(health_redis, state_redis):
    # type: (redis.Redis, redis.Redis) -> None
    for dev in get_health_devices(health_redis):
        state = state_redis.hget(STATE_HASH_KEY, dev) or "NULL"
        print("{} : {}".format(state, dev))


def publish_daqctl_state(control_redis, state, services, instances):
    # type: (redis.Redis, str, str, str) -> None
    command = {
        "command": "change_state",
        "value": state,
        "services": [services],
        "instances": [instances],
    }

    payload = json.dumps(command)
    print("PUBLISH {} {}".format(DAQCTL_CHANNEL, payload))
    control_redis.publish(DAQCTL_CHANNEL, payload)


def find_recbe_sampler_key_by_ip(parameter_redis, ip_address):
    # type: (redis.Redis, str) -> str
    for key in parameter_redis.scan_iter(match=RECBE_SAMPLER_KEY_PATTERN):
        value = parameter_redis.hget(key, RECBE_SAMPLER_IP_FIELD)
        if value == ip_address:
            if key.startswith(RECBE_SAMPLER_KEY_PREFIX):
                return key[len(RECBE_SAMPLER_KEY_PREFIX):]
            return key

    return ""


def make_ip_address_from_field(field):
    # type: (str) -> str
    try:
        number = int(field, 10)
    except ValueError as exc:
        raise ValueError("field is not an integer: {}".format(field)) from exc

    ip_suffix = number + 16
    if not 0 <= ip_suffix <= 255:
        raise ValueError(
            "field + 16 is outside IPv4 octet range: field={} suffix={}".format(
                field,
                ip_suffix,
            )
        )

    return "{}{}".format(RECBE_SAMPLER_IP_PREFIX, ip_suffix)


def run_script(script_args):
    # type: (list) -> None
    result = subprocess.run(
        [SCRIPT_PATH] + script_args,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )

    print(f"Return code: {result.returncode}")
    print("stdout:")
    print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
    print("stderr:")
    print(result.stderr, end="" if result.stderr.endswith("\n") else "\n")


def make_recbe_sampler_instance(field, parameter_redis):
    # type: (str, redis.Redis) -> str
    ip_address = make_ip_address_from_field(field)
    key = find_recbe_sampler_key_by_ip(parameter_redis, ip_address)
    if not key:
        raise ValueError(
            "RecbeSampler key not found: field={} ip={}".format(
                field,
                ip_address,
            )
        )

    return "RecbeSampler:{}".format(key)


def run_script_with_recbe_sampler_stop(
    script_args,
    control_redis,
    state_redis,
    parameter_redis,
    field,
):
    # type: (list, redis.Redis, redis.Redis, redis.Redis, str) -> None
    start_time = time.time()
    start_monotonic = time.monotonic()
    print(
        "run_script_with_recbe_sampler_stop start: {:.0f} {}".format(
            start_time,
            time.ctime(start_time),
        )
    )

    try:
        services = "RecbeSampler"
        instances = make_recbe_sampler_instance(field, parameter_redis)

        publish_daqctl_state(control_redis, "STOP", services, instances)
        #state_wait(control_redis, state_redis, "STOP")

        run_script(script_args)

        publish_daqctl_state(control_redis, "RUN", services, instances)
        time.sleep(5)
        state_wait(control_redis, state_redis, "RUN")
    finally:
        end_time = time.time()
        elapsed_ms = (time.monotonic() - start_monotonic) * 1000.0
        print(
            "run_script_with_recbe_sampler_stop end: {:.0f} {}".format(
                end_time,
                time.ctime(end_time),
            )
        )
        print(
            "run_script_with_recbe_sampler_stop elapsed_ms: {:.3f}".format(
                elapsed_ms
            )
        )


def parse_args():
    # type: () -> argparse.Namespace
    parser = argparse.ArgumentParser(
        description="Watch a Redis hash and execute a script when it changes.",
    )
    parser.add_argument(
        "--redis-host",
        default=REDIS_HOST,
        help=f"Redis host. Default: {REDIS_HOST}",
    )
    parser.add_argument(
        "--redis-port",
        type=int,
        default=REDIS_PORT,
        help=f"Redis port. Default: {REDIS_PORT}",
    )
    parser.add_argument(
        "--redis-db",
        type=int,
        default=REDIS_DB,
        help=f"Redis database number. Default: {REDIS_DB}",
    )
    parser.add_argument(
        "--consecutive-field-count",
        type=int,
        default=CONSECUTIVE_FIELD_COUNT,
        help=(
            "Number of consecutive updates for the same field before "
            f"executing the script. Default: {CONSECUTIVE_FIELD_COUNT}"
        ),
    )
    parser.add_argument(
        "--consecutive-field-timeout",
        type=float,
        default=CONSECUTIVE_FIELD_TIMEOUT_SECONDS,
        help=(
            "Maximum seconds allowed between consecutive updates for the "
            "same field. Default: {}".format(CONSECUTIVE_FIELD_TIMEOUT_SECONDS)
        ),
    )
    parser.add_argument(
        "--daqctl-services",
        default=DAQCTL_SERVICES,
        help=f"services value for daqctl change_state. Default: {DAQCTL_SERVICES}",
    )
    parser.add_argument(
        "--daqctl-instances",
        default=DAQCTL_INSTANCES,
        help=f"instances value for daqctl change_state. Default: {DAQCTL_INSTANCES}",
    )
    parser.add_argument(
        "--publish-daqctl-state",
        nargs=3,
        metavar=("STATE", "SERVICES", "INSTANCES"),
        help=(
            "Publish one daqctl change_state command and exit. "
            "Arguments: STATE SERVICES INSTANCES"
        ),
    )
    parser.add_argument(
        "--find-recbe-sampler-key",
        metavar="IP_ADDRESS",
        help=(
            "Find parameters:RecbeSampler-* in Redis DB 2 whose hash field "
            "'ip' matches IP_ADDRESS, print the key, and exit."
        ),
    )
    parser.add_argument(
        "--hlep",
        action="help",
        help=argparse.SUPPRESS,
    )
    return parser.parse_args()


def publish_daqctl_state_from_args(redis_host, redis_port, publish_args):
    # type: (str, int, list) -> None
    state, services, instances = publish_args
    control_r = redis.Redis(
        host=redis_host,
        port=redis_port,
        db=0,
        decode_responses=True,
    )
    publish_daqctl_state(control_r, state, services, instances)


def find_recbe_sampler_key_from_args(redis_host, redis_port, ip_address):
    # type: (str, int, str) -> None
    parameter_r = redis.Redis(
        host=redis_host,
        port=redis_port,
        db=RECBE_SAMPLER_DB,
        decode_responses=True,
    )
    key = find_recbe_sampler_key_by_ip(parameter_r, ip_address)
    if key:
        print(key)


def main(
    redis_host,
    redis_port,
    redis_db,
    consecutive_field_threshold,
    consecutive_field_timeout,
):
    # type: (str, int, int, int, float) -> None
    if consecutive_field_threshold < 1:
        raise ValueError("consecutive_field_threshold must be at least 1")
    if consecutive_field_timeout <= 0:
        raise ValueError("consecutive_field_timeout must be greater than 0")

    r = redis.Redis(
        host=redis_host,
        port=redis_port,
        db=redis_db,
        decode_responses=True,
    )
    control_r = redis.Redis(
        host=redis_host,
        port=redis_port,
        db=0,
        decode_responses=True,
    )
    state_r = redis.Redis(
        host=redis_host,
        port=redis_port,
        db=1,
        decode_responses=True,
    )
    parameter_r = redis.Redis(
        host=redis_host,
        port=redis_port,
        db=RECBE_SAMPLER_DB,
        decode_responses=True,
    )

    # Redis の keyspace notification を有効化
    r.config_set("notify-keyspace-events", "Kh")

    pubsub = r.pubsub()
    channel = f"__keyspace@{redis_db}__:{HASH_KEY}"
    pubsub.subscribe(channel)

    print(f"Watching Redis hash: {HASH_KEY}")
    print(f"Redis connection: host={redis_host} port={redis_port} db={redis_db}")
    print(f"Consecutive field threshold: {consecutive_field_threshold}")
    print(f"Consecutive field timeout: {consecutive_field_timeout} sec")

    last_hash = None
    last_changed_field = None
    last_changed_time = None
    consecutive_field_count = 0

    for message in pubsub.listen():
        if message["type"] != "message":
            continue

        event = message["data"]

        if event not in ("hset", "hmset", "hdel"):
            continue

        current_hash = r.hgetall(HASH_KEY)

        changed_fields = get_changed_fields(last_hash, current_hash)
        last_hash = current_hash

        if not changed_fields:
            continue

        for field, value in changed_fields:
            print(f"DB changed: field={field} value={value}")
            now = time.monotonic()

            if (
                field == last_changed_field
                and last_changed_time is not None
                and now - last_changed_time <= consecutive_field_timeout
            ):
                consecutive_field_count += 1
            else:
                last_changed_field = field
                consecutive_field_count = 1
            last_changed_time = now

            print(
                "Consecutive field count: "
                f"field={field} count={consecutive_field_count}/{consecutive_field_threshold} "
                f"timeout={consecutive_field_timeout} sec"
            )

            if consecutive_field_count < consecutive_field_threshold:
                continue

            if value is None:
                print(f"Skip script execution: field was deleted: {field}")
                consecutive_field_count = 0
                continue

            try:
                script_args = make_script_args({field: value})
            except ValueError as e:
                print(f"Skip hash update: {e}")
                consecutive_field_count = 0
                continue

            print(f"Execute: {SCRIPT_PATH} {' '.join(script_args)}")
            try:
                run_script_with_recbe_sampler_stop(
                    script_args,
                    control_r,
                    state_r,
                    parameter_r,
                    field,
                )
            except ValueError as e:
                print(f"Skip script execution: {e}")

            consecutive_field_count = 0


if __name__ == "__main__":
    args = parse_args()

    if args.publish_daqctl_state:
        publish_daqctl_state_from_args(
            args.redis_host,
            args.redis_port,
            args.publish_daqctl_state,
        )
        raise SystemExit(0)

    if args.find_recbe_sampler_key:
        find_recbe_sampler_key_from_args(
            args.redis_host,
            args.redis_port,
            args.find_recbe_sampler_key,
        )
        raise SystemExit(0)

    while True:
        try:
            main(
                args.redis_host,
                args.redis_port,
                args.redis_db,
                args.consecutive_field_count,
                args.consecutive_field_timeout,
            )
        except redis.exceptions.ConnectionError as e:
            print(f"Redis connection error: {e}")
            time.sleep(3)
