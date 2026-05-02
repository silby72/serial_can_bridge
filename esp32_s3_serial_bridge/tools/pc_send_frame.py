#!/usr/bin/env python3
"""
pc_send_frame.py
PC -> MCU へ送るシリアルフレーム送信ツール

フレーム構成 (serial_task.cpp に合わせる):
[START=0xAA][DEVICE_ID][LENGTH][DATA...][CHECKSUM]
- DEVICE_ID : 1 byte (受信側 MCU の DEVICE_ID と一致させる)
- LENGTH    : データバイト数 (DATA のバイト長)
- DATA      : int16 値の並び (big-endian, 上位バイト, 下位バイト)
- CHECKSUM  : ID ^ LEN ^ DATA[0] ^ DATA[1] ^ ...

使い方例:
  # 単発で送る（/dev/ttyUSB0, ボーレート115200, ID=0x00）
  python3 tools/pc_send_frame.py --port /dev/ttyUSB0 --id 0x00 --values 10,20

  # ループで送る（サーボ角 1 と 2 を交互に送る）
  python3 tools/pc_send_frame.py --port /dev/ttyUSB0 --id 0x00 --period 0.1 --loop --values 30,60

"""
import argparse
import serial
import struct
import time
import sys

START_BYTE = 0xAA

def to_int16_bytes(val):
    # pack big-endian signed 16-bit
    return struct.pack('>h', int(val))

def build_frame(device_id, int16_list):
    data_bytes = b''.join(to_int16_bytes(v) for v in int16_list)
    length = len(data_bytes)
    checksum = device_id ^ length
    for b in data_bytes:
        checksum ^= b
    frame = bytes([START_BYTE, device_id, length]) + data_bytes + bytes([checksum & 0xFF])
    return frame


def parse_values(s):
    if not s:
        return []
    parts = s.split(',')
    vals = []
    for p in parts:
        p = p.strip()
        if p == '':
            continue
        vals.append(int(p))
    return vals


def main():
    parser = argparse.ArgumentParser(description='Send binary frames to MCU (serial_bridge format)')
    parser.add_argument('--port', required=True, help='/dev/ttyUSB0 など')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--id', type=lambda x: int(x,0), default=0x00, help='DEVICE_ID (例: 0x00)')
    parser.add_argument('--values', type=str, default='', help='カンマ区切りの int16 値（例: 10,20, -100）')
    parser.add_argument('--period', type=float, default=1.0, help='ループ周期(秒), --loop 時のみ有効')
    parser.add_argument('--loop', action='store_true', help='period 毎に繰り返し送信する')
    parser.add_argument('--once', action='store_true', help='1回だけ送信して終了')
    parser.add_argument('--sin', action='store_true', help='sin波を生成して送信する')
    parser.add_argument('--freq', type=float, default=0.5, help='sin波周波数 [Hz]')
    parser.add_argument('--amplitude', type=int, default=30, help='sin波振幅 (int16)')
    parser.add_argument('--offset', type=int, default=90, help='sin波オフセット (int16)')
    parser.add_argument('--channels', type=int, default=2, choices=[1,2], help='生成するチャネル数 (1 or 2)')
    parser.add_argument('--sample-interval', type=float, default=0.02, help='送信間隔(秒) for sin/loop')
    parser.add_argument('--readback', action='store_true', help='シリアル受信を表示する(双方向)')
    args = parser.parse_args()

    vals = parse_values(args.values)
    if len(vals) == 0:
        print('Warning: 送信する値が空です。--values で int16 値を指定してください。')

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except Exception as e:
        print('シリアルポートを開けません:', e)
        sys.exit(1)

    try:
        if args.sin:
            import math
            print('Sending sin wave: freq={}Hz amp={} offset={} channels={} interval={}s'.format(
                args.freq, args.amplitude, args.offset, args.channels, args.sample_interval))
            start = time.time()
            while True:
                t = time.time() - start
                vals_sin = []
                for ch in range(args.channels):
                    phase = 0.0 if ch == 0 else math.pi / 2.0
                    v = int(args.offset + args.amplitude * math.sin(2.0 * math.pi * args.freq * t + phase))
                    # clamp to int16
                    if v < -32768:
                        v = -32768
                    if v > 32767:
                        v = 32767
                    vals_sin.append(v)

                frame = build_frame(args.id & 0xFF, vals_sin)
                ser.write(frame)
                ser.flush()

                if args.readback:
                    # read and print any incoming bytes
                    while ser.in_waiting:
                        data = ser.read(ser.in_waiting)
                        print('RX:', data.hex(), repr(data))

                time.sleep(args.sample_interval)

        else:
            frame = build_frame(args.id & 0xFF, vals)
            print('Built frame:', frame.hex())

            if args.loop:
                print('Looping send every', args.period, 's. Ctrl-C to stop.')
                while True:
                    ser.write(frame)
                    ser.flush()
                    if args.readback:
                        while ser.in_waiting:
                            data = ser.read(ser.in_waiting)
                            print('RX:', data.hex(), repr(data))
                    time.sleep(args.period)
            else:
                ser.write(frame)
                ser.flush()
                print('Frame sent once')
    except KeyboardInterrupt:
        print('\nUser canceled')
    finally:
        ser.close()

if __name__ == '__main__':
    main()
