
[英語版(English)](README.md)

# はじめに

TD-USBは、東京デバイセズのUSB製品を制御するためのコマンドラインインタフェースです。

すぐに使ってみたい方は、[クイックスタートガイド](docs/quickstart_ja.md)を参照してください。


## 動作環境

- Microsoft Windows 7以降 64ビット
- Linux (Ubuntu LTS 20.04) 以降


## TD-USBの入手

### Windows

Windowsユーザは[TD-USBリリースページ](https://github.com/tokyodevices/td-usb/releases)からビルド済み実行ファイルをダウンロードしてください。
ZIPファイルをダウンロード後、任意のフォルダに解凍してください。
アンインストールする場合には、フォルダごと削除してください。

TD-USBの起動時に「VCRUNTIME140.dll がコンピュータ内に見つからないため、プログラムを起動できません」というエラーが発生する場合には、
[最新の Visual C++ 再頒布可能パッケージ](https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist?view=msvc-170)のx86版を
ダウンロード・インストールして再度お試しください。

### Linux

Linuxユーザの肩は下記の手順でソースコードからビルドしてください。
なお、ソースコードのビルド・コンパイルに関する知識やサポートは東京デバイセズのサポート対象外です。
バグレポート・要望等はGithubのIssueから追加をお願いいたします。

### ソースコードからビルド

**Windows**

ソースコードからコンパイルする場合には Microsoft Visual Studio が必要です。
TD-USBは `Setupapi.lib` と `Hid.lib` に依存しますので、これらのライブラリファイルがライブラリパスから見える必要があります。
ライブラリの入手方法についてはWindows Driver Kit や Windows SDK を参照してください。
また、ソースコードのうち/linuxフォルダに含まれるソースコードはコンパイル対象から除外してください。

**Linux**

Githubリポジトリ [tokyodevices/td-usb](https://github.com/tokyodevices/td-usb) より
ソースコードをcloneしてコンパイルをしてください。

TD-USBは `libusb-dev` パッケージに依存します。
ディストリビューションに応じて相当のパッケージを事前にインストールして、GCCのライブラリパスから見えるようにしてください。

   例) `apt install libusb-dev`

なお、多くのディストリビューションでは、一般ユーザがUSBデバイスにアクセスするためにudevの権限設定が必要になります。

例えばUbuntuやDebian系の場合 `/etc/udev/rules.d/99-usb-tokyodevices.rules` ファイルを作成し、
次の内容を記載してください:

    SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05df", MODE="0666"

上記の例の`16c0`や`05df`は制御したいデバイスのVID/PIDに変えてください。
VID/PIDはUSBデバイスを挿した際のdmesgやsyslogから確認可能です。


## 対応モデル


|製品型番|製品名|model_name文字列|
|-------|-----|---------------|
|IWT120-USB|[汎用 USB 警告ランプ・ブザー モジュール](https://tokyodevices.com/items/201)|`iwt120`|
|IWT1320-USB|[ＬＥＤ・ブザー付　汎用ＵＳＢスライダー入力デバイス](https://tokyodevices.com/items/271)|`iwt1320`|
|IWS600-CM|[USB 焦電型赤外線 汎用人感センサ 5m バスパワー (コマンド制御タイプ)](https://tokyodevices.com/items/177)|`iws600cm`|
|IWS660-CS|[汎用 USB 照度センサ, 照度計, 光量計, バスパワー, 10 to 60,000 Lx](https://tokyodevices.com/items/228)|`iws660`|
|IWS73X-CS|[USB CO2(二酸化炭素)濃度・温度・湿度センサ 「エア・アナライザ」](https://tokyodevices.com/items/205)|`iws73x`|
|IWT303-1C|[USBリレー制御モジュール 1接点 10A 250V](https://tokyodevices.com/items/148)|`iwt303`|
|IWT303-3C|[USBリレー制御モジュール 3接点 10A 250V](https://tokyodevices.com/items/149)|`iwt303`|
|IWT313-USB|[USBリレー制御モジュール 8接点 6A 250V](https://tokyodevices.com/items/207)|`iwt313`|
|TDFA30301|[USBリレー制御ユニット 1接点 10A 250V AC 24V DC バスパワー](https://tokyodevices.com/items/358)|`tdfa30301`|
|TDFA30303|[USBリレー制御ユニット 3接点 10A 250V AC 24V DC バスパワー](https://tokyodevices.com/items/359)|`tdfa30303`|
|TDFA30608|[USB絶縁デジタルIO入力モジュール 8接点 電流ソース・シンク対応型](https://tokyodevices.com/items/284)|`tdfa30608`|
|TDFA30604|USB絶縁デジタルIO入力モジュール 4接点 電流ソース・シンク対応型|`tdfa30604`|
|TDFA50507|[USB絶縁デジタルIO出力モジュール 7接点 電流シンクタイプ](https://tokyodevices.com/items/308)|`tdfa50507`|
|TDFA50607|[USB絶縁デジタルIO出力ボード 5/12/24V出力 7接点 電流ソースタイプ](https://tokyodevices.com/items/345)|`tdfa50607`|
|TDFA60220|[絶縁型 4-20mA アナログ信号対応 USB電流計・ADコンバータ](https://tokyodevices.com/items/311)|`tdfa60220`|
|TDFA60228|(仮) 絶縁型 8ch 4-20mA アナログ信号対応 USB電流計・ADコンバータ|`tdfa60228`|
|TDFA60250|[絶縁型 0-5V 1-5V アナログ信号対応 USB電圧計・ADコンバータ](https://tokyodevices.com/items/314)|`tdfa60250`|
|TDFA6032A|[0-20mA/4-20mA電流信号用 USBアナログ出力ユニット 絶縁タイプ](https://tokyodevices.com/items/315)|`tdfa6032`|
|TDFA60350|[USB 0-5V/1-5V 電圧信号用 アナログ出力ユニット ガルバニック絶縁 バスパワー](https://tokyodevices.com/items/323)|`tdfa60350`|
|TDPC0201|[USBウォッチドッグ・電源リセットデバイス 「リセットマスター」](https://tokyodevices.com/items/288)|`tdpc0201`|
|TDPC0205|["時計ドングル" USB リアルタイムクロック(RTC)  現在時刻取得・同期用 電池バックアップ](https://tokyodevices.com/items/319)|`tdpc0205`|
|TDPC2401|USB挿抜コントローラ基板|`tdpc2401`|
|TDPW6620|USB LED定電流コントローラ、1ch・最大1 A|`tdpw6620`|
|TDSN0700-UA|[USB 汎用 紫外線照度計, UVセンサ, UVA 295-490nm, バスパワー](https://tokyodevices.com/items/321)|`tdsn0700`|
|TDSN0700-UB|[USB 汎用 紫外線照度計, UVセンサ, UVB 240-320nm, バスパワー](https://tokyodevices.com/items/322)|`tdsn0700`|
|TDSN5200|[汎用 USB ToF レーザー距離センサ 距離計測 最大6m バスパワー](https://tokyodevices.com/items/326)|`tdsn5200`|
|TDSN604A8|[汎用 高精度 USB 電流圧センサ 20ビット ±0.8A ガルバニック絶縁 バスパワー](https://tokyodevices.com/items/332)|`tdsn604xx`|
|TDSN60408|[汎用 高精度 USB 電流圧センサ 20ビット ±8A ガルバニック絶縁 バスパワー](https://tokyodevices.com/items/331)|`tdsn604xx`|
|TDSN60420|[汎用 高精度 USB 電流圧センサ 20ビット ±20A ガルバニック絶縁 バスパワー](https://tokyodevices.com/items/330)|`tdsn604xx`|
|TDSN7200|[汎用 高精度 USB温度・湿度・気圧センサ, -40 to 125℃, 0 to 100%RH, 260 to 1,260hPa, バスパワー](https://tokyodevices.com/items/327)|`tdsn7200`|
|TDSN7300|[USB CO2(二酸化炭素)濃度・温度・湿度センサ 「エア・アナライザ2」](https://tokyodevices.com/items/356)|`tdsn7300`|
|TDSN7360|[汎用 高精度 USB 加速度センサ 3軸 低ノイズ ±2.5g 440Hz バスパワー](https://tokyodevices.com/items/335)|`tdsn7360`|
|TDSN7400|[汎用 USB 熱電対 温度センサ K型 -40 to 1200℃ ガルバニック絶縁 バスパワー](https://tokyodevices.com/items/333)|`tdsn7400`|
|TDSN7502|[汎用 ロードセル to USB インタフェース 重量センサ 圧力センサ 重さセンサ 24ビット バスパワー](https://tokyodevices.com/items/344)|`tdsn7502`|
|TDFA1104|[汎用 USB 7セグメント LED 表示器 4桁 文字高0.8インチ 赤色](https://tokyodevices.com/items/350)|`tdfa1104`|

※TD-USBに対応しないUSB製品については、各製品ごとに制御プログラムが提供されています。
詳しくは東京デバイセズ各製品のページ・マニュアルを参照してください。


## コマンド仕様

TD-USBコマンドの書式は次の通りです。

    td-usb model_name[:serial] operation [options]
    
`td-usb`はコマンド名です。

`model_name[:serial]`には、操作対象の製品モデル名とシリアル番号を指定します。モデル名の指定は必須です。
モデル名とシリアル番号はコロン記号で結合します。
それぞれ大文字小文字が区別されます。
シリアル番号を省略し、モデル名のみを指定するとPCに接続されたデバイスのうち、OSが最初に列挙した個体が操作対象となります。

`operation`はオペレーションを指定するための固定文字列です。指定は必須です。
`list`, `get`, `set`, `listen`, `save`, `destroy` が指定できます。
製品ごとに対応しているオペレーションは異なります。

`[options]`はオペレーションごとに定義されるオプションです。


### list オペレーション

`list`オペレーションはPCに接続されたデバイスのシリアル番号を列挙します。
PCに同じモデルの製品が複数台接続されている時に各個体のシリアル番号を知るために使用します。
`list`オペレーションについては、`model_name[:serial]`にシリアル番号を指定しても無視されます。

実行すると、検出されたデバイスのシリアル番号を列挙した文字列が標準出力に1行書き出されます。
各シリアル番号はカンマ文字で結合されます。
シリアル番号の文字数・書式は製品ごとに異なります。また、出力順番の保証はありません。
デバイスが見つからなかった場合には空行が出力されます。


    例: ABCD123456,EFGH123456,IJKLM123456


### get オペレーション

`get`オペレーションは指定されたデバイスのデバイスレジスタから値を取得します。

    td-usb model_name[:serial] set (name)


`(name)`は取得対象のデバイスレジスタ名です。製品によっては省略可能です。

実行すると、取得された値を含む文字列が1行出力されます。
名称および文字列の仕様は製品ごとのマニュアルを参照してください。

`--format=(simple|json|csv)`オプションは、値の出力形式を指定するオプションです。
`simple`, `json`, `csv`を指定できます。
`simple`は通常必要となる情報のみを出力します(デフォルト)。
`json`は読み取った値のうち、すべての情報をJSON形式で出力します。
`csv`は読み取った値のうち、すべての情報をカンマ区切りで出力します。
製品ごとに対応するフォーマットの値が異なります。各製品のマニュアルを参照してください。

`--loop[=delay]`オプションを指定すると、一定時間ごとに繰り返し値を取得します。
このオプションを指定しない場合には1回値を取得するとコマンドは終了します(デフォルト)。
`delay`には繰り返しの時間間隔をミリ秒単位で指定します。
なお、`--loop`のみを記述した場合には1000ミリ秒間隔となります。

### listen オペレーション

`listen`オペレーションは、デバイスイベントを待機します。


    td-usb model_name[:serial] listen (name)


`(name)`は待機するデバイスイベントを決定するための名称です。製品によっては省略可能です。
コマンドを実行するとデバイスイベントが発生するまで待機します。
デバイスイベントが発生するタイミングや、発生した際に表示される文字列の仕様は製品ごとのマニュアルを参照してください。


### set オペレーション

`set`オペレーションは指定されたデバイスレジスタに値を設定します。


    td-usb model_name[:serial] set (name)=(value)

`(name)`はデバイスレジスタ名、`(value)`は値です。
設定名と設定値については製品ごとのマニュアルを参照してください。

`set`オペレーションに`--loop`オプションを指定して起動すると、`(name)=(value)`を標準入力から繰り返し読み込みます。
値を連続して指定する場合に便利です。


    td-usb model_name[:serial] set --loop


### save オペレーション

`save`オペレーションは`set`により設定された値が揮発性である場合、デバイス内部の不揮発性メモリに保存します。
保存された値は、次回デバイスを起動する際にデフォルト値として使用されます。
製品によっては`set`オペレーションで値を設定した時点で不揮発性メモリに保存され、`save`オペレーションが必要ない場合があります。


### destroy オペレーション

`destroy`オペレーションは、対応するデバイスの場合、デバイスのファームウェアを自己消去してデバイスの機能を停止させます。
ファームウェアをアップデートする際に使用します。

**注意:** このオペレーションを実行すると、**新しいファームウェアが書き込まれるまでデバイスは動作しなくなります**のでご注意ください。
ファームウェアアップデートの前段階としてファームウェアの消去が必要になる場合のみ使用してください。
誤ってファームウェアを削除された場合には修理サポート(有償)となります。

コマンドを実行すると、コマンドラインに次の警告メッセージが表示されます。
ファームウェアを消去する場合には`y`キーを入力してください。
実行しない場合には`y`以外のキーを入力してください。

    WARNING: The device will not be available until new firmware is written. Continue? [y/N]


### TDFA60228 8ch電流入力

レジスタ名なしの`get`はCH0～CH7を順に取得し、カンマ区切りの1行で出力します。
レジスタ名を指定した場合は、指定順に出力します。既定の表示では各チャンネルの
校正値を用いて`電流[mA] = ADC_VALUE_CHn × 25.0 / CALIBRATION_CHn`で換算します。
4～20mAの範囲への制限は行いません。`--format=raw`は校正値を読まず、ADC生値を
符号なし整数で出力します。それ以外のレジスタは整数で表示しますが、
`FIRMWARE_VERSION`は既定で`major.minor`、rawで格納された整数値を表示します。

|レジスタ名|アドレス|アクセス・SETの有効値|`save`の対象|
|---|---|---|---|
|`CONTROL_FLAG`|`0x00`|読書き可。0: DUMP無効、4: DUMP有効。初期値0|対象|
|`CALIBRATION_CH0`～`CALIBRATION_CH7`|`0x01`～`0x08`|読書き可。1～4095。初期値3103|対象|
|`ADC_VALUE_CH0`～`ADC_VALUE_CH7`|`0x80`～`0x87`|読出し専用。0～4095。`0xFFFF`: 測定値なし|対象外|
|`FIRMWARE_VERSION`|`0xF2`|読出し専用。`(major << 8) \| minor`|対象外|

CH0～CH7は基板のIN0～IN7に対応します。校正値は、各チャンネルに25mAを入力した
ときのADC生値です。SETには符号なし10進整数を指定します。予約ビット、範囲外の値、
読出し専用レジスタへの書込みはエラーになります。全引数を検証してから指定順に
書き込み、保存には別途`save`を実行します。単一チャンネル用の`ADC_VALUE`と
`CALIBRATION`という名前はTDFA60228では使用できません。

`listen`はDUMPを受信するたびにCH0～CH7を1行で出力します。`ADC_VALUE_CHn`を
指定すると、そのチャンネルのみを指定順に出力します。先に`set CONTROL_FLAG=4`で
DUMPを有効にしてください。`listen`自体は設定を変更せず、イベントが来るまで待機します。
ADC通信失敗時も含めて100ms周期でDUMPを送信します。DUMPの8chは同じ走査の結果ですが、
個別のGETでは走査の更新をまたぐ場合があります。各chは順番に測定され、同時測定ではありません。
`--skip=N`は完全なDUMPをN件読み飛ばしてから次の1行を出力し、チャンネル単位では
間引きません。`--loop`なしでも1行を出力するまで待機します。
停止には`set CONTROL_FLAG=0`を使用します。既定表示とrawに対応し、
JSON・CSV・TSVのフォーマット指定には対応しません。

初回測定成功前またはADC通信失敗時、ファームウェアは全chに`0xFFFF`（65535）を返します。
`get`または`listen`がこの値を受信すると、標準エラー出力に未測定またはADC通信失敗を示す
メッセージを出し、終了コード12（`DEVICE_IO_ERROR`）で終了します。`--format=raw`や
`--loop`でも同じ動作です。異常を検出した行は、一部分も標準出力に出しません。
`listen`は`--skip`による間引きの前に全chを検証するため、読み飛ばすDUMPの異常も検出します。

校正値はデバイス接続ごとに保持し、このプロセスで該当する校正レジスタを書き換えた
後に再取得します。別プロセスで校正値を変更した場合、継続中のget/listenは再起動してください。
不正な校正値やDUMPは通信エラーとなります。raw表示に校正値は不要です。

```text
td-usb tdfa60228 get
td-usb tdfa60228 get ADC_VALUE_CH0 ADC_VALUE_CH7 --format=raw
td-usb tdfa60228 get FIRMWARE_VERSION CALIBRATION_CH7
td-usb tdfa60228 set CALIBRATION_CH0=3103 CALIBRATION_CH7=3098
td-usb tdfa60228 save
td-usb tdfa60228 set CONTROL_FLAG=4
td-usb tdfa60228 listen --loop
td-usb tdfa60228 listen ADC_VALUE_CH0 ADC_VALUE_CH7 --format=raw --skip=9 --loop
td-usb tdfa60228 set CONTROL_FLAG=0
```

`init`と`dfu`は未対応です。`destroy`はファームウェアのRelease版にある消去処理を
呼び出します。現在のファームにはブートローダがないため、復旧にはSWD書込みが必要であり、
USB DFUモードには入りません。Debug版のファームはERASEを受け付けません。


### TDPC2401 USB挿抜コントローラ

レジスタ名なしの `get` は `BRIDGE_STATUS` を返します。複数のレジスタ名を指定すると、
指定順の符号なし10進数をカンマ区切りで出力します。標準形式と `--format=raw` は同じ出力です。
`set 0` ～ `set 3` は `set BRIDGE_STATUS=0` ～ `set BRIDGE_STATUS=3` の省略形です。
設定値は符号なし10進数で指定します。

|レジスタ|範囲・意味|`save` の保存対象|
|---|---|---|
|`INITIAL_BRIDGE_STATUS`|0～3、起動時の接続状態。初期値3|対象|
|`AUTO_ENABLE`|0: 停止、1: 開始・再開始。初期値0|対象|
|`AUTO_MASK`|1～3、自動運転の切断対象。初期値3|対象|
|`CONNECTED_TIME`|1～4294967295秒。初期値3595|対象|
|`DISCONNECTED_TIME`|1～4294967295秒。初期値5|対象|
|`BRIDGE_STATUS`|0～3、現在の接続状態|対象外|
|`TIMER_REMAINING`|残り秒数。読み取り専用、停止中は0|対象外|
|`FIRMWARE_VERSION`|`(major << 8) \| minor`。読み取り専用|対象外|

接続状態とマスクの bit 0 は VBUS、bit 1 は DATA です。接続状態では1が接続を表します。
`BRIDGE_STATUS` への書き込みは自動運転を停止します。
`AUTO_ENABLE=0` は現在の接続状態を維持して停止し、1の書き込みは接続区間の先頭から開始します。
設定は指定順に適用されるため、自動運転の開始は最後に指定してください。

```text
td-usb tdpc2401 get BRIDGE_STATUS TIMER_REMAINING --loop=1000
td-usb tdpc2401 set BRIDGE_STATUS=3
td-usb tdpc2401 set AUTO_ENABLE=0 AUTO_MASK=3 CONNECTED_TIME=3595 DISCONNECTED_TIME=5 AUTO_ENABLE=1
td-usb tdpc2401 save
```

`init` は自動運転を停止し、上記の初期値へ戻して VBUS と DATA の両方を接続します。
設定を保存する場合は別途 `save` を実行してください。手動の `BRIDGE_STATUS` は保存されず、
起動時は `AUTO_ENABLE` と `INITIAL_BRIDGE_STATUS` に従います。
`listen` と `destroy` は非対応です。状態監視には `get --loop` を使用します。


### 終了コードとエラー

TD-USBコマンドは正常に終了すると終了コード0を返します。
0以外の場合にはエラーによる異常終了です。値と意味は次の通りです。
エラーの詳細は標準エラー出力に書き出されます。


|コード|名称                        |説明                                                       |
|------|----------------------------|-----------------------------------------------------------|
|0     |NO_ERROR                    |正常にプロセスが終了しました.                              |
|2     |UNKNOWN_DEVICE              |不明な`model_name`が指定されました.                      |
|3     |UNKNOWN_OPERATION           |不明な`operation`が指定されました.                       |
|4     |OPERATION_NOT_SUPPORTED     |指定された`operation`は指定デバイスでは非対応です.        |
|6     |INVALID_OPTION              |オプションの指定が不正です.                                |
|11    |DEVICE_OPEN_ERROR           |デバイスのオープンに失敗しました.                          |
|12    |DEVICE_IO_ERROR             |デバイスとの通信に失敗しました.                            |
|13    |INVALID_FORMAT              |`--format`で指定された値は指定デバイスでは利用できません. |
|13    |INVALID_FORMAT              |`--format`で指定された値は指定デバイスでは利用できません. |
|14    |INVALID_RANGE               |与えられた数値の範囲が不正です. |

## ユーザアプリケーションとの統合

ユーザ独自のアプリケーションから東京デバイセズのUSBデバイスを利用するには、
アプリケーション側から **TD-USBコマンドを外部プログラムとして呼び出す** 必要があります。

TD-USBコマンドは、標準出力に結果を書き出します。
アプリケーションから結果を知りたい場合には、
外部プログラムとして呼び出す際にパイプによって標準出力を受け取るようにしてください。

### 別のコマンドにパイプで接続する例

td-usbコマンドによりIWS450センサの値を1秒ごとにyour-applicationに対して出力

    %  td-usb iws450 get --loop=1000 | your-application


### Pythonからの呼び出し例

subprocessを使用し、外部プログラムとしてTD-USBコマンドを起動してください。

listオペレーションの例:

    import subprocess
    output = subprocess.check_output(['td-usb','tdfa30608','list'])
    print(output)


getオペレーションの例:

    import subprocess

    command = ['td-usb', 'tdfa30608', 'get', '--loop=100']

    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

    for line in process.stdout:
        print(line, end='')

    for line in process.stderr:
        print(line, end='')

    process.wait()


### C# からの呼び出し例

ProcessおよびProcessStartInfoを利用してプロセスを作成し、標準出力をリダイレクトすることで結果を読み取ることができます。

    ProcessStartInfo psInfo = new ProcessStartInfo();
    psInfo.FileName = "C:\path_to_td-usb\td-usb.exe";
    psInfo.CreateNoWindow = true;
    psInfo.UseShellExecute = false;
    psInfo.RedirectStandardOutput = true;
    Process p = Process.Start(psInfo);
    string output = p.StandardOutput.ReadToEnd(); 
    Debug.Write(output);

### LabVIEW からの呼び出し

[接続]»[ライブラリと実行可能ファイル]パレットにあるSystem ExecVIを使用することで実現可能です。TD-USBを呼び出し、"標準出力"から結果を受け取ってください。

### その他のプログラム

各言語からの外部プログラムの起動方法や、標準入出力のリダイレクト方法について、各マニュアルを参照してください。


## ライセンス

TD-USB のライセンスは [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) です。

