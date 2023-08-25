---
title: KEK Online Class Library Design Principle
...

------------------------------------------------------------------------

<div align="center">

network データ収集用C++ class libraryの設計方針
-----------------------------------------------

(©2006 Hirofumi Fujii, KEK Online-electronics group)\
(Revised: 24-Aug-2006)

</div>

------------------------------------------------------------------------
------------------------------------------------------------------------

### はじめに

この文書は、著者が製作した network データ収集用C++ class library
の設計方針について 述べたものである。

### 設計方針

TCP stream や Thread を扱う C++ class library
は世の中に既にある。そこで、
本パッケージでは特に凝ったことはしない。できるだけ単純に C の socket
library や thread library を class 化するにとどめる。

一方 class 化するに当たり、model としたのは
、（実行効率はともかく）programming しやすい と言われている Java
である。特に"JAVAプログラム　クイック　リファレンス"に掲載されて いる
network の例題がほぼそのまま焼き直せることを強く意識した。

上記二つの要求を満たすために

-   Socket を扱う class：これは単純に BSD socket library の socket
    descriptor を隠蔽する だけに留める。
-   TCP stream を扱う class：これは socket
    をメンバーとして持ち、インターフェースは 標準 library の
    iostreamに近づける。もしくは iostream から派生させる。
-   Thread を扱う class、これも単純に pthread の start routine と thread
    descriptor を 隠蔽するだけに留める。
-   Thread の start routine の隠蔽の仕方は、Java の runnable class を
    model とする。

------------------------------------------------------------------------

### Socket に関する class

Socket class に関しては、socket descriptor
を隠蔽することを目的とする。従って、用意される member
関数は原則として、BSD socket library の socket descriptor
を除いたものとする。

Socket descriptor は BSD socket library の socket()
関数を使って生成するが、 BSD socket library にはもう一つ socket
descriptor を生成する accept() 関数がある。 これは stream 型で listen
している socket から生成される。これをどう反映させるかが class
設計に大きく影響する。大雑把に次の二つの方法が考えられる。

-   空の Socket() を new して、そこへ accept した descriptor
    を設定し、その pointer を返す。
-   accept で作成された descriptor を含む Socket そのものを返す。

前者の場合、実装は容易である。またメモリの利用効率もよい。但し、ユーザ側では
受け取った pointer に対し、使用が終わったら必ず delete
する必要が生じる。これを忘れると descriptor
の未解放が生じる。一方後者の方法では、代入演算子の実装が不可欠となる。
代入を実装するためには socket descriptor
のコピーが必要であり、descriptor 資源を
浪費することになる。資源的には無駄が多いと思われる方法であるが、scope
からはずれると 自動的に desctructor が呼ばれるので、destructorで close
するようにしておけば、ユーザ側での
誤操作は少なくなると期待される。また、thread
の独立性を高めるためにも、pointer ではなく Socket そのものを thread
に持たせる方が扱いやすいと考えられる。ここでは後者の方法をとった。

### TCP stream に関する class

[前のページ](index.htm)に述べたように、本class は標準libraryのiostreamに
近づけることを指針とする。iostreamから派生させることもできているが、現時点の配布パッケージには
含まれていない。従って、現時点ではmanipulatorの入出力先としては指定できない。

Socket の生成法の違いにより

-   *TCPClient*　：socket()+connect()で生成
-   *TCPServer*　：socket()+bind()で生成
-   *TCPSocket*　：accept()で生成

の３つのclassを用意した。このうち*TCPClinet*及び*TCPSocket*はともに
iostream的
な入出力を行うので、*TCPBuffer*というclassを用意し、それから派生させている。
iostream の用法に合わせるため、std::ios class にある
iostateフラグに関する関数のうち

-   operator void\*() const
-   bool operator!() const
-   bool good() const
-   bool eof() const
-   bool fail() const
-   bool bad() const

を用意した。

入力に関して std::istream class との対応を以下の表に示す。

  ------------------------------------------------------------------------
  member function
  implemented

  streamsize gcount() const
  yes

  int get()
  yes

  istream& get(char& c)
  no

  istream& get(char\* s, streamsize n)
  no

  istream& get(char\* s, streamsize n, char delim)
  no

  istream& getline(char\* s, streamsize n)
  yes

  istream& getline(char\* s, streamsize n, char delim)
  no

  istream& ignore(streamsize n=1, int delim=traits::eof())
  delim 無しで実装

  int peek()
  no

  istream& read(char\* s, streamsize n)
  yes

  int readsome(char\* s, streamsize n)
  no

  istream& putback(char c)
  no

  istream& unget()
  no

  int sync()
  yes
  ------------------------------------------------------------------------

出力に関しては std::ostream class との対応は以下の通り。

  ------------------------------------------------------------------------
  member function
  implemented

  ostream& put(char c)
  yes

  ostream& write(const char\* s, streamsize n)
  yes

  ostream& flush()
  yes
  ------------------------------------------------------------------------

------------------------------------------------------------------------

### Thread 関連 class

*Thread* class に関しては、start routine と thread desciptor
を隠蔽する。 start routine を隠蔽する理由は、一般に start routine は
global object であり、thread で操作したい data object
との結びつきをユーザが設定しなければならないために、
ユーザにとって必ずしも使いやすくないからである。

C++ はデータとその操作関数を一体的に
記述できることが利点の一つであるので、これを一体的に扱える設計にした。
この model として Java の runnable class 的に使える設計にした。

ユーザは基本的に Thread として実行したい関数及びその時に必要となる data
をひとまとめにして Thread class から派生させた class をつくり、

-   Thread として実行される int run() 関数を上書きする。
-   Thread class の member 関数 start() を呼び出す。

とすればよい。

Thread 間での排他制御や同期をとるために、*Mutex* と *Semaphore* の
二つの class を用意した。これらはいずれも POSIX の mutex と semaphore
に対応 させてある。

------------------------------------------------------------------------
------------------------------------------------------------------------
