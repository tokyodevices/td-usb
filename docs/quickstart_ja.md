# 実行例

## コマンドの実行方法

### Windows

Windowsに標準で付属されている
[Windows Power Shell](https://docs.microsoft.com/ja-jp/powershell/) などからコマンドを実行してください。

    % cd (TD-USBをインストールしたフォルダ)
    % .\td-usb

※TD-USBはコマンドラインプログラムのため、マウスでtd-usb.exeファイルをクリックして実行することはできません.

### Linux

Bash等のターミナルからコマンドを実行してください。

## 値を読み取る

例: TDFA30608から最新のポートの状態を1回読み出す

    > tdusb tdfa30608 read
    3

## デバイスのシリアル番号を取得する

同じ製品が複数台PCに接続されている場合には、以下のコマンドで各デバイスのシリアル番号を取得できます。

    > tdusb tdfa30608 list
    1234ABCD,5678EFGH
    
上記の例では、PCに2台のTDFA30608モジュールが接続されていて、それぞれ`1234ABCD`と`5678EFGH`のシリアル番号を持つことがわかります。

## シリアル番号を指定して値を読み取る

    > tdusb tdfa30608:1234ABCD read
    3

## 5秒ごとに値を読み取る

    > tdusb tdfa30608 read --loop=5000
    3
    3
    3
    3
    ...

