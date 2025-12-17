#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AXON実機結合テスト自動化スクリプト
金額・面番号変更シーケンステスト (TEST-AXON-001)

使用方法:
    python test_axon_money_face_change.py --port COM3

必要なライブラリ:
    pip install pyserial colorama
"""

import serial
import struct
import time
import argparse
import sys
from datetime import datetime
from typing import Optional, Tuple
from colorama import init, Fore, Back, Style

# Colorama初期化
init(autoreset=True)

# CRC16計算（ISO/IEC 13239準拠）
def crc16_tep(data: bytes) -> int:
    """CRC16計算（SOMA-AXON仕様準拠）"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0x8408
            else:
                crc >>= 1
    crc ^= 0xFFFF
    # バイトスワップ
    return ((crc & 0xFF) << 8) | ((crc >> 8) & 0xFF)


class AXONTester:
    """AXON基板テスタークラス"""
    
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0):
        """
        Args:
            port: シリアルポート名 (例: COM3, /dev/ttyUSB0)
            baudrate: ボーレート（デフォルト: 115200）
            timeout: タイムアウト時間（秒）
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser: Optional[serial.Serial] = None
        self.test_results = []
        
    def connect(self) -> bool:
        """シリアルポート接続"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout
            )
            print(f"{Fore.GREEN}✓ シリアルポート接続成功: {self.port} @ {self.baudrate}bps")
            time.sleep(0.5)  # 接続安定化待ち
            return True
        except serial.SerialException as e:
            print(f"{Fore.RED}✗ シリアルポート接続失敗: {e}")
            return False
    
    def disconnect(self):
        """シリアルポート切断"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print(f"{Fore.YELLOW}シリアルポート切断")
    
    def send_chkirq(self, port_num: int = 1) -> bool:
        """
        CHKIRQコマンド送信
        
        Args:
            port_num: ポート番号（1 or 2）
        
        Returns:
            送信成功: True, 失敗: False
        """
        # CHKIRQ平文32バイト構築
        chkirq_plain = bytearray(32)
        chkirq_plain[0] = 0x49  # ID
        # RFU[1-25] = 0x00
        chkirq_plain[26:28] = struct.pack('<H', 0x0123)  # DIC (LE)
        chkirq_plain[28:30] = struct.pack('<H', 0x0000)  # AuthCode
        chkirq_plain[30:32] = struct.pack('<H', 0x0000)  # RND
        
        # フレーム構築: Header(0x14) + LEN(0x20) + Data(32) + CRC(2)
        frame = bytearray([0x14, 0x20])
        frame.extend(chkirq_plain)
        crc = crc16_tep(frame)
        frame.extend(struct.pack('<H', crc))
        
        # 送信
        self.ser.write(frame)
        print(f"{Fore.CYAN}→ CHKIRQ送信 ({len(frame)}バイト)")
        return True
    
    def receive_atirq(self) -> Optional[dict]:
        """
        ATIRQ応答受信
        
        Returns:
            受信データ辞書 or None（タイムアウト時）
        """
        try:
            # Header + LEN読み取り
            header = self.ser.read(2)
            if len(header) < 2:
                print(f"{Fore.RED}✗ ATIRQ受信タイムアウト")
                return None
            
            if header[0] != 0x14 or header[1] != 0x20:
                print(f"{Fore.RED}✗ ATIRQ ヘッダー不正: {header.hex()}")
                return None
            
            # Data(32) + CRC(2)読み取り
            data_crc = self.ser.read(34)
            if len(data_crc) < 34:
                print(f"{Fore.RED}✗ ATIRQ データ不足")
                return None
            
            # CRC検証
            frame = header + data_crc[:32]
            crc_recv = struct.unpack('<H', data_crc[32:34])[0]
            crc_calc = crc16_tep(frame)
            
            if crc_recv != crc_calc:
                print(f"{Fore.RED}✗ ATIRQ CRCエラー (受信: 0x{crc_recv:04X}, 計算: 0x{crc_calc:04X})")
                return None
            
            # データ解析
            plain = data_crc[:32]
            result = {
                'id': plain[0],
                'mode': plain[1],
                'face_n': plain[2] & 0x0F,
                'cash_vlu': struct.unpack('<H', plain[3:5])[0],
                'status': struct.unpack('<H', plain[5:7])[0],
                'msn': plain[7:13].hex(),
                'afw_ver': plain[13],
                'chk_led': plain[14],
                'chk_tout': plain[15],
            }
            
            print(f"{Fore.GREEN}← ATIRQ受信成功")
            print(f"   FACE_N: {result['face_n']}, CASH_VLU: {result['cash_vlu']}×100円")
            print(f"   MODE: 0x{result['mode']:02X}, STATUS: 0x{result['status']:04X}")
            
            return result
            
        except Exception as e:
            print(f"{Fore.RED}✗ ATIRQ受信エラー: {e}")
            return None
    
    def send_setaxon(self, face_n: int, set_face_n: int, set_cash_vlu: int,
                     set_sol: int = 0, set_led: int = 0, set_tout: int = 0) -> bool:
        """
        SETAXONコマンド送信
        
        Args:
            face_n: 現在のFACE番号 (0-9)
            set_face_n: 設定FACE番号 (0-9)
            set_cash_vlu: 設定金額 (100円単位、0-99)
            set_sol: ソレノイド設定 (bit0: ソレノイド, bit4: 現金ブロック)
            set_led: LED設定 (bit0-2: RGB, bit4-6: 動作モード)
            set_tout: タイムアウト設定 (0-14)
        
        Returns:
            送信成功: True, 失敗: False
        """
        # SETAXON平文32バイト構築
        setaxon_plain = bytearray(32)
        setaxon_plain[0] = 0x4A  # ID
        setaxon_plain[1] = face_n & 0x0F
        setaxon_plain[2] = set_sol
        setaxon_plain[3] = set_led
        setaxon_plain[4] = set_tout
        # RFU1[5-6] = 0x00
        setaxon_plain[7] = set_face_n & 0x0F
        setaxon_plain[8:10] = struct.pack('<H', set_cash_vlu)
        # RFU2[10-25] = 0x00
        setaxon_plain[26:28] = struct.pack('<H', 0x0123)  # DIC
        setaxon_plain[28:30] = struct.pack('<H', 0x0000)  # AuthCode
        setaxon_plain[30:32] = struct.pack('<H', 0x0000)  # RND
        
        # フレーム構築: Header(0x14) + LEN(0x20) + Data(32) + CRC(2)
        frame = bytearray([0x14, 0x20])
        frame.extend(setaxon_plain)
        crc = crc16_tep(frame)
        frame.extend(struct.pack('<H', crc))
        
        # 送信
        self.ser.write(frame)
        print(f"{Fore.CYAN}→ SETAXON送信 (FACE: {face_n}→{set_face_n}, CASH: {set_cash_vlu}×100円)")
        return True
    
    def receive_ack_nack(self) -> Optional[str]:
        """
        ACK/NACK応答受信
        
        Returns:
            'ACK', 'NACK', or None（タイムアウト時）
        """
        try:
            # Header読み取り
            header = self.ser.read(1)
            if len(header) < 1:
                print(f"{Fore.RED}✗ ACK/NACK受信タイムアウト")
                return None
            
            if header[0] == 0x10:
                # ACK応答（36バイト）
                rest = self.ser.read(35)
                if len(rest) < 35:
                    print(f"{Fore.RED}✗ ACKデータ不足")
                    return None
                print(f"{Fore.GREEN}← ACK受信")
                return 'ACK'
                
            elif header[0] == 0x90:
                # NACK応答（5バイト）
                rest = self.ser.read(4)
                if len(rest) < 4:
                    print(f"{Fore.RED}✗ NACKデータ不足")
                    return None
                err_code = rest[1]
                print(f"{Fore.RED}← NACK受信 (エラーコード: 0x{err_code:02X})")
                return 'NACK'
            else:
                print(f"{Fore.RED}✗ 不明な応答ヘッダー: 0x{header[0]:02X}")
                return None
                
        except Exception as e:
            print(f"{Fore.RED}✗ ACK/NACK受信エラー: {e}")
            return None
    
    def wait_for_button_press(self, button_name: str, count: int = 1):
        """ボタン押下待機（手動操作）"""
        print(f"\n{Back.YELLOW}{Fore.BLACK} 【操作待ち】 {Style.RESET_ALL}")
        print(f"   {button_name}を{count}回押してください")
        input(f"   押下後、Enterキーを押してください... ")
    
    def run_test_case(self, tc_id: str, description: str, 
                      face_before: int, cash_before: int,
                      face_after: int, cash_after: int,
                      button_operations: str) -> bool:
        """
        テストケース実行
        
        Args:
            tc_id: テストケースID
            description: テスト項目名
            face_before: 変更前の面番号
            cash_before: 変更前の金額
            face_after: 変更後の面番号
            cash_after: 変更後の金額
            button_operations: ボタン操作内容
        
        Returns:
            テスト成功: True, 失敗: False
        """
        print(f"\n{'='*70}")
        print(f"{Fore.CYAN}{Style.BRIGHT}テストケース: {tc_id} - {description}")
        print(f"{'='*70}")
        print(f"変更前: FACE={face_before}, CASH={cash_before}×100円")
        print(f"変更後: FACE={face_after}, CASH={cash_after}×100円")
        print(f"操作: {button_operations}")
        
        # ボタン押下待機
        self.wait_for_button_press(button_operations)
        
        # CHKIRQ送信
        time.sleep(0.1)
        if not self.send_chkirq():
            return False
        
        # ATIRQ受信
        time.sleep(0.1)
        atirq = self.receive_atirq()
        if not atirq:
            return False
        
        # SETAXON送信
        time.sleep(0.1)
        if not self.send_setaxon(
            face_n=atirq['face_n'],
            set_face_n=face_after,
            set_cash_vlu=cash_after
        ):
            return False
        
        # ACK/NACK受信
        time.sleep(0.1)
        response = self.receive_ack_nack()
        
        # 結果判定
        success = (response == 'ACK' and 
                   atirq['face_n'] == face_after and
                   atirq['cash_vlu'] == cash_after)
        
        if success:
            print(f"{Fore.GREEN}{Style.BRIGHT}✓ テスト成功")
        else:
            print(f"{Fore.RED}{Style.BRIGHT}✗ テスト失敗")
            print(f"   期待値: FACE={face_after}, CASH={cash_after}")
            print(f"   実測値: FACE={atirq['face_n']}, CASH={atirq['cash_vlu']}")
        
        # 結果記録
        self.test_results.append({
            'tc_id': tc_id,
            'description': description,
            'success': success,
            'timestamp': datetime.now().isoformat()
        })
        
        return success
    
    def print_summary(self):
        """テスト結果サマリー表示"""
        print(f"\n{'='*70}")
        print(f"{Fore.CYAN}{Style.BRIGHT}テスト結果サマリー")
        print(f"{'='*70}")
        
        total = len(self.test_results)
        passed = sum(1 for r in self.test_results if r['success'])
        failed = total - passed
        
        for result in self.test_results:
            status = f"{Fore.GREEN}✓ PASS" if result['success'] else f"{Fore.RED}✗ FAIL"
            print(f"{status} {result['tc_id']}: {result['description']}")
        
        print(f"\n{'-'*70}")
        print(f"合計: {total} | 成功: {Fore.GREEN}{passed}{Style.RESET_ALL} | "
              f"失敗: {Fore.RED}{failed}{Style.RESET_ALL}")
        
        if failed == 0:
            print(f"\n{Fore.GREEN}{Style.BRIGHT}🎉 全テストケース合格！")
        else:
            print(f"\n{Fore.RED}{Style.BRIGHT}⚠ {failed}件のテストが失敗しました")


def main():
    """メイン関数"""
    parser = argparse.ArgumentParser(
        description='AXON実機結合テスト自動化スクリプト',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('--port', '-p', required=True,
                        help='シリアルポート名 (例: COM3, /dev/ttyUSB0)')
    parser.add_argument('--baudrate', '-b', type=int, default=115200,
                        help='ボーレート (デフォルト: 115200)')
    parser.add_argument('--timeout', '-t', type=float, default=2.0,
                        help='タイムアウト時間[秒] (デフォルト: 2.0)')
    
    args = parser.parse_args()
    
    print(f"{Fore.CYAN}{Style.BRIGHT}")
    print("="*70)
    print("  AXON実機結合テスト自動化スクリプト")
    print("  金額・面番号変更シーケンステスト (TEST-AXON-001)")
    print("="*70)
    print(Style.RESET_ALL)
    
    # テスター初期化
    tester = AXONTester(args.port, args.baudrate, args.timeout)
    
    # 接続
    if not tester.connect():
        sys.exit(1)
    
    try:
        # テストケース実行
        print(f"\n{Fore.YELLOW}注意: ボタン操作は手動で行ってください")
        print(f"      各テストケース実行前に操作指示が表示されます")
        
        # TC-7.4-01: 初期設定+金額変更
        tester.run_test_case(
            tc_id='TC-7.4-01',
            description='初期設定+金額変更',
            face_before=0, cash_before=0,
            face_after=1, cash_after=2,
            button_operations='面番号ボタン1回 + 金額ボタン2回'
        )
        
        # TC-7.4-02: 金額変更（複数回）
        tester.run_test_case(
            tc_id='TC-7.4-02',
            description='金額変更（複数回）',
            face_before=1, cash_before=2,
            face_after=1, cash_after=4,
            button_operations='金額ボタン2回'
        )
        
        # TC-7.5-01: 0面設定（メンテナンス）
        tester.run_test_case(
            tc_id='TC-7.5-01',
            description='0面設定（メンテナンス）',
            face_before=1, cash_before=4,
            face_after=0, cash_after=0,
            button_operations='面番号ボタン9回（1→0に巡回）'
        )
        
        # TC-7.4-03: 金額変更禁止（0面時）
        print(f"\n{Fore.YELLOW}注意: このテストでは金額変更が禁止されることを確認します")
        tester.run_test_case(
            tc_id='TC-7.4-03',
            description='金額変更禁止（0面時）',
            face_before=0, cash_before=0,
            face_after=0, cash_after=0,
            button_operations='金額ボタン1回（禁止確認）'
        )
        
        # TC-7.5-03: 0面から通常面への復帰
        tester.run_test_case(
            tc_id='TC-7.5-03',
            description='0面から通常面への復帰',
            face_before=0, cash_before=0,
            face_after=4, cash_after=2,
            button_operations='面番号ボタン4回 + 金額ボタン2回'
        )
        
        # 結果サマリー表示
        tester.print_summary()
        
    except KeyboardInterrupt:
        print(f"\n{Fore.YELLOW}テスト中断")
    
    finally:
        # 切断
        tester.disconnect()


if __name__ == '__main__':
    main()
