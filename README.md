# Arduino_DAC

	このスケッチに対するシリアル通信は'j'で始まり'x'で終える
	数値があるべき場所にアルファベットなどがある場合，予期せぬ動作をする
	（atoi()，atol()で文字列を数値に変換している）

	シリアル通信テスト用
	    j8012340678955555x
	    j1143211987600000x
	    j0000000000010000x


	シリアルバッファが16バイトである場合にバッファを変数に格納
	[mode] [forward/velocity] [crosswise/radius] [delay]
	[0] [0 0000] [0 0000] [00000]
	[mode]
	0 : 即時停止
	1 : 電圧のシリアル値（相対値，スムージングなし）
	2 : 電圧のシリアル値（相対値，スムージングあり）
	3 : ルンバ式シリアル値

	[mode == 0]
	続く15バイトを無視して電圧を標準値にする
	滑らかに停止させる場合，[2 00000 00000 xxxxx]

	[mode == 1 || mode == 2] [forward] [crosswise]
	-4095 ~ 04095
	基準電圧 + (-5.00 ~ 5.00) [V]
	ただし，結果が 0 ~ 4095 を逸脱しない
	符号は0かそれ以外かで判別する

	[mode == 3] [velocity] [radius]
	-9999 ~ 09999
	速度は[mm/s]，半径は[mm]
	±5000を上限の目安にする
	後ろ向きに走行する場合，速度が負のとき進行方向，半径が負のとき進行方向左回り
	符号は0かそれ以外かで判別する

	[delay]
	00000 ~ 99999
	受信したシリアル値を出力してからdelay[ms]維持する
	その後，新しい指令値がない場合は停止
	割り込む場合は[mode == 0]で停止させてから新しいデータを送信する



