#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
仕様書の図(Figure)を画像リンクに置き換えるスクリプト
"""

import re

def update_soma_axon():
    """SOMA-AXON仕様書の図を更新"""
    print('=== SOMA-AXON仕様書を更新中 ===')

    with open('SOMA-AXON仕様書_ver0.1.md', 'r', encoding='utf-8') as f:
        content = f.read()

    # Figure 2-1
    content = content.replace(
        '下記のFigure 2-1にシステム構成図を示します。\n\n\n\nFigure 2-1, システム構成図\n\n',
        '下記のFigure 2-1にシステム構成図を示します。\n\n![Figure 2-1](images_axon/figure_01.png)\n\n**Figure 2-1, システム構成図**\n\n'
    )

    # Figure 2-2
    content = content.replace(
        '![alt text](image-1.png)\nFigure 2-2, ThincaGateとSOMA基板とAXON基板の接続概略図\n\n![alt text](image-2.png)\n',
        '![Figure 2-2](images_axon/figure_02.png)\n\n**Figure 2-2, ThincaGateとSOMA基板とAXON基板の接続概略図**\n\n'
    )

    # Figure 2-3
    content = content.replace(
        '![alt text](image-3.png)\nFigure 2-3, カプセルトイとAXON基板間の接続概略図\n\n',
        '![Figure 2-3](images_axon/figure_03.png)\n\n**Figure 2-3, カプセルトイとAXON基板間の接続概略図**\n\n'
    )

    # Figure 3-1
    content = content.replace(
        '![alt text](image-4.png)\nFigure 3-1, キャラクタフォーマット\n\n',
        '![Figure 3-1](images_axon/figure_04.png)\n\n**Figure 3-1, キャラクタフォーマット**\n\n'
    )

    with open('SOMA-AXON仕様書_ver0.1.md', 'w', encoding='utf-8') as f:
        f.write(content)

    print('✅ SOMA-AXON仕様書の図を更新しました')

def update_soma_tg():
    """SOMA-TG仕様書の図を更新"""
    print('\n=== SOMA-TG仕様書を更新中 ===')

    with open('SOMA-TG仕様書_ver1.0.md', 'r', encoding='utf-8') as f:
        content = f.read()

    # Figure 2-1
    content = content.replace(
        '下記の\n\nFigure 2-1にシステム構成図を示します。\n\n\n\n\nFigure 2-1, システム構成図\n\n',
        '下記のFigure 2-1にシステム構成図を示します。\n\n![Figure 2-1](images_tg/figure_01.png)\n\n**Figure 2-1, システム構成図**\n\n'
    )

    # Figure 2-2
    content = content.replace(
        '\n\nFigure 2-2, ThincaGateとSOMA基板とAXON基板の接続概略図\n\n',
        '\n\n![Figure 2-2](images_tg/figure_02.png)\n\n**Figure 2-2, ThincaGateとSOMA基板とAXON基板の接続概略図**\n\n'
    )

    # Figure 2-3
    content = content.replace(
        '\n\nFigure 2-3, カプセルトイとAXON基板間の接続概略図\n\n',
        '\n\n![Figure 2-3](images_tg/figure_03.png)\n\n**Figure 2-3, カプセルトイとAXON基板間の接続概略図**\n\n'
    )

    # Figure 3-1 (SOMA-TGもAXONと同じキャラクタフォーマット図を使用)
    content = content.replace(
        '\n\nFigure 3-1, キャラクタフォーマット\n\n',
        '\n\n![Figure 3-1](images_axon/figure_04.png)\n\n**Figure 3-1, キャラクタフォーマット**\n\n'
    )

    with open('SOMA-TG仕様書_ver1.0.md', 'w', encoding='utf-8') as f:
        f.write(content)

    print('✅ SOMA-TG仕様書の図を更新しました')

if __name__ == '__main__':
    print('仕様書の図を画像リンクに置き換えます...\n')
    print('注意: 実行前に仕様書ファイルを保存して閉じてください。\n')

    input('準備ができたらEnterキーを押してください...')

    update_soma_axon()
    update_soma_tg()

    print('\n✅ すべての更新が完了しました！')
    print('\n更新内容:')
    print('- Figure 2-1, 2-2, 2-3: システム構成図と接続図')
    print('- Figure 3-1: キャラクタフォーマット図')
    print('\nVS Codeで仕様書を開いて、画像が正しく表示されることを確認してください。')
