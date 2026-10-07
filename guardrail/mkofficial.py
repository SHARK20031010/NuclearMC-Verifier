#!/usr/bin/env python3
"""从出厂基线快照生成「Geant4 官方物理列表清单」。

用途：护栏要判断一个程序的物理列表是不是官方列表之一。判据是
**构造器集合**（不是名字 —— 程序里那个名字字符串可能是随便起的、
也可能因为直接 new 类而根本没有名字）。

输入：一个目录，里面是出厂基线快照 JSON（默认 ../g4dump/out/base）
输出：data/official_lists.json  {列表名: [构造器名, ...]}

为什么需要这个文件：用户在程序里手工拼物理列表时，程序不会报错、
名字也看不出问题，但物理内容与任何官方列表都不同（实测 24 个任务里
有 13 个属于这种）。这类决定必须能被查出来。
"""
import json, os, sys, glob

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'g4dump', 'out', 'base')
OUT = os.path.join(HERE, 'data', 'official_lists.json')


def main():
    official = {}
    sigs = {}          # frozenset(ctors) -> [列表名...]
    for p in sorted(glob.glob(os.path.join(SRC, '*.json'))):
        name = os.path.splitext(os.path.basename(p))[0]
        try:
            s = json.load(open(p, encoding='utf-8'))
        except Exception as e:                       # 半截文件跳过
            print(f'跳过 {name}: {e}', file=sys.stderr)
            continue
        ctors = sorted(set(s.get('constructors', [])))
        if not ctors:
            print(f'跳过 {name}: 没有构造器', file=sys.stderr)
            continue
        official[name] = ctors
        sigs.setdefault(frozenset(ctors), []).append(name)

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    json.dump(official, open(OUT, 'w', encoding='utf-8'),
              ensure_ascii=False, indent=1, sort_keys=True)

    print(f'写出 {OUT}')
    print(f'官方列表 {len(official)} 个；不同构造器集合 {len(sigs)} 种')
    dup = {k: v for k, v in sigs.items() if len(v) > 1}
    if dup:
        print('★ 构造器集合相同、只有名字不同的列表（说明「靠名字判断」本身就不可靠）：')
        for k, v in dup.items():
            print('   ', ' = '.join(v), f'（{len(k)} 个构造器）')
    else:
        print('没有构造器集合重复的列表。')
    sizes = sorted((len(v), k) for k, v in official.items())
    print('构造器最少的 5 个官方列表：', ', '.join(f'{k}({n})' for n, k in sizes[:5]))
    print('构造器最多的 5 个官方列表：', ', '.join(f'{k}({n})' for n, k in sizes[-5:]))


if __name__ == '__main__':
    main()
