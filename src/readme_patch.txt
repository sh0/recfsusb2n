差分のみのアーカイブです。オリジナルに上書して下さい。


[変更点]
・recpt1ctlに対応
・チューニング完了判定の数値を変更（"pDev->DeMod_GetSequenceState() < 8"の8を9に）
・受信開始時安定化待ち処理追加
・httpサーバ機能移植(--http portnumber)
・tssplitter_liteを内臓(--sid n1,n2,.. number,all,hd,sd1,sd2,sd3,1seg,epg)
・受信開始前にウェイトを入れるオプションを追加(--wait n[1=100mSec])


[ライセンス・再配布]
tssplitter_liteとhttpサーバーについては該当部分はそれぞれのものに準拠
他の部分は、本家に準拠　ただし商利用については本家の明言があるまで禁止とします。
また本家に取り込まれた場合は、その時点で当方の権利を本家に移譲します。

