#!/bin/sh

COMMENT="[PC-8801] Ys  - Ancient Ys Vanished Omen -"
COMPOSER="古代祐三"
AUTHOR="日本ファルコム"
DATE="1987/06/21"

YK2="古代祐三"
ISH="石川三恵子"
YOU="永田英哉"
ABE="阿部隆人"
HSH="橋本昌哉"

DATA=data
MUC=muc

conv()
{
  ./ys12muc -n $1 -T $3 -o $MUC/${2}_$1.MUC -d "$DATE" -a "$AUTHOR" -C "$COMMENT" -c "$4" -t "$5" $DATA/$2
}

mkdir $MUC
conv 0 TTLMUS    4 "$YK2" "Feena"
conv 0 MUSD10A   1 "$ISH" "Fountain of Love"
conv 1 MUSD10A   1 "$ABE" "Templo del Sol"
conv 0 MUSD11    1 "$YK2" "The SYONIN"
conv 1 MUSD11    1 "$YK2" "Lucky"
conv 2 MUSD11    1 "$YK2" "Tears of Sylph"
conv 0 MUSD20    1 "$YK2" "First Step towards wars"
conv 1 MUSD20    1 "$YK2" "Lucky"
conv 0 MUSD12    1 "$YK2" "Palace"
conv 1 MUSD12    1 "$YK2" "Lucky"
conv 0 DKMUS1A   1 "$YK2" "Holders of power"
conv 1 DKMUS1A   1 "$YK2" "Boss out"
conv 2 DKMUS1A   1 "$YK2" "Door open"
conv 0 MUSD80    1 "$YK2" "Palace of Destruction"
conv 1 MUSD80    1 "$YK2" "Lucky"
conv 0 MUSD50    1 "$YK2" "Beat of the Terror"
conv 1 MUSD50    1 "$YK2" "Lucky"
conv 0 GOVMUSA   1 "$YK2" "Game over"
conv 0 MUSD10B   4 "$YK2" "Tower of the Shadow of Death"
conv 1 MUSD10B   4 "$YK2" "Lucky"
conv 2 MUSD10B   4 "$YK2" "Devil's wind"
conv 0 MUSD90    4 "$YK2" "The last moment of the Dark"
conv 1 MUSD90    4 "$YK2" "Lucky"
conv 0 DKMUS1B   4 "$YK2" "Holders of power"
conv 1 DKMUS1B   4 "$YK2" "Boss out"
conv 0 DKMUS2    4 "$YK2" "Final Battle"
conv 1 DKMUS2    4 "$YK2" "Boss out"
conv 0 GOVMUSB   4 "$YK2" "Game over"
conv 0 MUSDA0    4 "$YK2" "Rest in Peace"
conv 1 MUSDA0    4 "$YK2" "Lucky"
conv 0 ENDMUS    4 "$YK2" "The morning grow"
conv 1 ENDMUS    4 "$YK2" "See you again"
conv 0 YMUS16    4 "$HSH" "Fair wind"
conv 0 YMUS39    4 "$ISH" "Shining star"
conv 0 YMUS10    4 "$YK2" "Dreaming"
conv 0 YMUS08    4 "$YK2" "Chase of shadow"
conv 0 YMUS35    4 "$YK2" "over drive"
conv 0 YMUS06    4 "$YK2" "NHK教育"
conv 0 YMUS19    4 "$YK2" "旅立ち"
conv 0 YMUS18    4 "$YK2" "悲しみクロスロード"
conv 0 YMUS07    4 "$YK2" "Battle ground"
conv 0 YMUS09    4 "$YK2" "Mysterious moment"
conv 0 YMUS42    4 "$YK2" "アドルのテーマ"
conv 0 YMUS36    4 "$YK2" "Dead-end street"
conv 0 YMUS13    4 "$YK2" "Sub-mission"
