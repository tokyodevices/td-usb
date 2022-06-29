# はじめに

TD-USBは、東京デバイセズのUSB製品を制御するためのコマンドラインインタフェースです。

- [英語版(English)](README.md)
- [クイックスタートガイド](docs/quickstart_ja.md)


## 動作環境

- Microsoft Windows 7以降 32/64ビット  
  (Microsoft Windows XP SP3 **非推奨**)
- Linux (Ubuntu LTS 20.04)


## ビルドとダウンロード

ソースコードのビルド・コンパイルに関する知識やサポートは東京デバイセズのサポート対象外です。
バグレポート・要望等はGithubのIssueから追加をお願いいたします。

### Windows

[TD-USBリリースページ](https://github.com/tokyodevices/td-usb/releases)

上記リンクより最新バージョンのZIPファイルをダウンロード後、任意のフォルダに解凍してください。
アンインストールする場合には、フォルダごと削除してください。

ソースコードからコンパイルする場合には Microsoft Visual Studio が必要です。
TD-USBは `Setupapi.lib` と `Hid.lib` に依存しますので、
これらのライブラリファイルがライブラリパスから見える必要があります。
ライブラリの入手方法についてはWindows Driver Kit や Windows SDK を参照してください。

### Linux

Githubリポジトリ [tokyodevices/td-usb](https://github.com/tokyodevices/td-usb) より
ソースコードをcloneしてコンパイルをしてください。

TD-USBは `libusb-dev` パッケージに依存します。
ディストリビューションに応じて相当のパッケージを事前にインストールして、GCCのライブラリパスから見えるようにしてください。

   例) `apt install libusb-dev`



## 対応モデル


|製品型番|製品名|model_name文字列|
|-------|-----|---------------|
|IWT120-USB|[汎用 USB 警告ランプ・ブザー モジュール](https://tokyodevices.com/items/201)|`iwt120`|
|IWS660-CS|[汎用 USB照度センサ (明るさセンサ)](https://tokyodevices.com/items/228)|`iws660`|
|IWT303-1C|[USBリレー制御モジュール 1接点 10A 250V](https://tokyodevices.com/items/148)|`iwt303`|
|IWT303-3C|[USBリレー制御モジュール 3接点 10A 250V](https://tokyodevices.com/items/149)|`iwt303`|
|IWT313-USB|[USBリレー制御モジュール 8接点 6A 250V](https://tokyodevices.com/items/207)|`iwt313`|
|TDFA30608|[USBデジタルIO入力モジュール 8接点 電流ソース・シンク対応型](https://tokyodevices.com/items/284)|`tdfa30608`|
|TDPC0201|[USBウォッチドッグ・電源リセットデバイス 「リセットマスター」](https://tokyodevices.com/items/288)|`tdpc0201`|

※TD-USBに対応しないUSB製品については、各製品ごとに制御プログラムが提供されています。
詳しくは東京デバイセズ各製品のページ・マニュアルを参照してください。


## コマンド仕様

TD-USBコマンドの書式は次の通りです。

    td-usb model_name[:serial] operation [options]
    
`td-usb`はコマンド名です。

`model_name[:serial]`には、操作対象の製品モデル名とシリアル番号を指定します。モデル名の指定は必須です。
モデル名とシリアル番号はコロン記号で結合します。
それぞれ大文字小文字が区別されます。
シリアル番号を省略し、モデル名のみを指定するとPCに接続された最初のデバイスが操作対象となります。

`operation`はオペレーションを指定するための固定文字列で、`list`, `get`, `set`, `listen`, `save`, `destroy` が指定できます。
指定は必須です。

`[options]`は製品モデルごとに定義されたオプションです。詳しくは各製品のマニュアルを参照してください。

### list オペレーション

`list`はPCに接続されたデバイスのシリアル番号を列挙します。
PCに同じモデルの製品が複数台接続されている時に使用します。
`model_name[:serial]`にはモデル名のみを指定してください。

実行すると、検出されたデバイスのシリアル番号を列挙した文字列が1行出力されます。
各シリアル番号はカンマ文字で結合されます。(順番の保証はありません。)

    例: ABCD123456,EFGH123456,IJKLM123456


### get オペレーション

`get`は指定されたデバイスから値を取得します。

実行すると、取得された値を含む文字列が1行出力されます。
文字列の内容は製品ごとに異なります。 各製品のマニュアルを参照してください。

`--format=(simple|json|csv)`オプションは、値の出力形式を指定するオプションです。
`simple`, `json`, `csv`を指定できます。
`simple`は通常必要となる情報のみを出力します(デフォルト)。
`json`は読み取った値のうち、すべての情報をJSON形式で出力します。
`csv`は読み取った値のうち、すべての情報をカンマ区切りで出力します。
製品ごとに対応するオプションの値が異なります。各製品のマニュアルを参照してください。

`--loop[=delay]`オプションを指定すると、一定時間ごとに繰り返し値を取得します。
このオプションを指定しない場合には1回値を取得するとコマンドは終了します(デフォルト)。
`delay`には繰り返しの時間間隔をミリ秒単位で指定します。
なお、`--loop`のみを記述した場合には1000ミリ秒間隔となります。

### set オペレーション

`set`は指定されたデバイスに値を設定します。対応製品のみ指定可能です。

### listen オペレーション

`listen`は、デバイスからイベントを取得します。
コマンドを実行するとイベントが発生するまで待機します。
イベントが発生するタイミングは製品ごとに異なります(センサの値が変化した場合など)。
対応製品のみ指定可能です。

### save オペレーション

`save`はデバイスに設定された値をデバイス内部のフラッシュメモリに書き込みます。
書き込まれた値は、次回デバイスを起動する際にデフォルト値として使用されます。対応製品のみ指定可能です。

### destroy オペレーション

`destroy`は、現在書き込まれているデバイスのファームウェアを消去してデバイスの機能を停止させます。対応製品のみ指定可能です。

**注意:** このオペレーションを実行すると、**新しいファームウェアが書き込まれるまでデバイスは動作しなくなります**のでご注意ください。
ファームウェアアップデートの前段階としてファームウェアの消去が必要になる場合のみ使用してください。
誤ってファームウェアを削除された場合には修理サポート(有償)となります。

コマンドを実行すると、コマンドラインに次の警告メッセージが表示されます。
ファームウェアを消去する場合には`y`キーを入力してください。
実行しない場合には`y`以外のキーを入力してください。

    WARNING: The device will not be available until new firmware is written. Continue? [y/N]


### 終了コード

TD-USBコマンドは正常に終了すると終了コード0を返します。
0以外の場合にはエラーによる異常終了です。
エラーの情報は標準エラー出力に書き出されます。



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

    import subprocess
    output = subprocess.check_output(['td-usb','tdfa30608','list'])
    print(output)


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

### その他のプログラム

各言語からの外部プログラムの起動方法や、標準入出力のリダイレクト方法について、各マニュアルを参照してください。
