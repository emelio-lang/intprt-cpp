<TODO>
U-22提出まで
9/1
・notationが書ける
9/2
・subtract関数を追加してやったら死ぬ
・fuse関数
let if = (fuse (|0 _ f| f)
               (|pred t _| t)
               )

・型システム
・fuse, notationをもっと自由な関数らしい関数に
(これは無理かも)・C or C++に変換して簡易コンパイラ
それ以降
・importシステム
・メモリ見直し
・boostを使ってみる
・その他実用において欲しいシステム...


(add (negate 5) (negate 5))

type Either(a) = a | Null

grow: Seed -> Either(Vegetable)


<Ordinary> Bind nomi
a 3 3;
b 6;
c 2

|
v

notation A ; B = (|x| B) (A)

(|x| c 2) ((|x| b 6) (a 3 3))




let :: Bind #Symbol a -> a Bind
    :: Bind /#Symbol a -> a/

(|x| (let (get_bind x) b 2)) (let _bind a 3)

(add (negate 2) (negate 3))

(|bind| (|bind| add a b) (let a 3)) (let b 8)

Bind { let b 3
     ; let a 3
     ; add a b
     }


Bindは変数があるなら全て必要。

積型を[a1 a2 a3 a4...]みたいに書くことにすると、
add [negate 1 negate 3]
のようになる。関数適用のspaceと、積型のspaceが一緒なので見にくい気もするし、関数の横だけ見れば良いので大丈夫という気もする。
ex)
add [negate 1 3]
add [add [1 3] 3]

(|x y| x + y) [1 3]


(|x| let [get_bind x b 2]) let [_bind a 3]

(add [negate 2 negate 3])

(|bind| (|bind| add [a b]) let [a 3]) let [b 8]

Bind { let [b 3]
     ; let [a 3]
     ; add [a b]
     }
いろいろ書いてみて分かったけど、やっぱりわかりにくい。関数名が変数っぽかったら終わる
foo [x a b c] -- 完全に４つ撮ってるように見えるけど、実際は3つ(x:fn)
foo (x a) b c -- よくわかる（括弧とかの最初が関数）

add [negate 1 negate 3]



<関数適用>
このプログラミング言語においてエッセンシャルな部分としては集合（関数としても実装可能）と関数になる。
関数は作ることが出来て、適用することができれば十分だ。
なので、まず作り方と適用のやりかたを表す関数を作る。（ここがややこしい、循環？？？）

作り方(記法)
(|引数| 返り値)

適用とは
(|x y| x + y) 1 2 --> 1 + 2
文字列的な処理なので、
apply: (a -> b) a -> b
apply: #Fn a -> #Fn
と二通りに書ける。前者は書いたはそこまでで実装は出来ないけど、後者は実装することが出来る。
この違いは何だろうか.

<文字列でないと表現できないプログラムがあった>
それはそう。ただ、ラムダ計算の理論が言っていることは、関数の生成と適用さえあればOKということだと思うので（違う？？）
これ以外の意味を変更するようなnotationを禁止するようにする。もっと言えば、関数の適用に対するnotationしか認めないようにするとOKということOK。


<本質的な循環>
コンパイラ・インタプリタという代物について考えてみる。
コンパイラは[#Symbol] ->



composition: (b -> c) (a -> b) -> (a -> c) = (|g f| g f)
(composition inc negate) 3 -- -2
inc negate 3 -- -2

は？ラムダ抽象を使う.

composition: (b -> c) (a -> b) -> (a -> c) = \g f -> \x -> g (f x)
(composition inc negate) 3 -- -2
(\x -> inc (negate x)) 3
inc (negate 3)


<関数の記法についての考察>

Haskellでは関数を
\引数 -> 項
のように書いている。

Emelioでは
(|引数| 項)
のように書く予定だったが、実際何が一番良いのか考えてみる。

<<<括弧をつけることが意味することとは>>>
括弧、つけすぎるとlispみたいになってミスが起こったり書きにくかったりするのも確かです。変更もやりにくい。
どんな時に括弧がいるのでしょう。
ずばり、範囲を決めないといけないときです。始点・終点のみを決めないといけない時には括弧は必要ありません。
この場合は


((|f g| (|x| f (g x))) negate negate) 3

(add 2 3)

add
   2  3
   
(|x y| add x y)
 ((|z1| negate z1) 3)
 ((|z2| 3) (add 2 3))

<<<簡約>>>
基本的にやることは変数の置き換え。


<<<シンボルを取る関数は必要か>>>
(notation (A ; B) ((|x| B) (A)) (
add 3 2;
add 4 7
))


<<<言語の指針>>>
・何が悪い言語か
まずは自分の意見を書く。
プログラミング言語は目的があって使われる。当然選択するプログラミング言語も、今書こうと思っているプログラムによって使い分ける。
今回作ろうと思っている言語は汎用言語である。色々な目的で使えるぜ！っていう言語。
では、プログラミング言語がそのプログラムにマッチしているとはどのようなことを言うのか。
たとえば、「競技プログラミングに向いている言語」といったときには。
「競プロで書くプログラムの集合」にたいして、それぞれその言語で表現をした時に、
その長さとか可読性、書きやすさ、Customizabilityとか色々あるんでしょう。
なので、評価関数E(r)を定義しておくと、
Σ　E(r in 競プロで書くプログラムの表現の集合)
みたいなものが大きいプログラミング言語のことを言うのでしょう。

ではこれをどうやって測るのかと言うのが問題になってくる。結局そこが人間によるのでしょう。

色々な人の意見を聞く。
[https://medium.com/smalltalk-talk/the-three-worst-programming-languages-b1ec25a232c1]



<<[Bytes]を取る関数はできるか、作ったほうが良いか>>
プログラムなど、PCの全てはByte * Byte * ... = [Byte]として表現されます。
型なし言語はこのようなこの型しか搭載していない言語と言えます。
今のところは[Byte]しか実装していないので、この上で考えていきます。型の導入仕方についても考えたいと思います。

さて、今作っているのは関数として捉えるとどのような関数化でしょう？
インタプリタは関数を簡約化する作業です。

parser: [Byte] -> (a -> a)
interpreter: World (a -> a) -> World (a -> a)
applier: a (a -> a) -> a
ref: [Byte] -> (a -> a)

ここで、今私たちは記法について扱いたいわけです。
parserをかましてからは全て関数の表現になっているので、parserのような関数となるわけです。

作って良いでしょう。

<<優先順位、結合方向>>
大体の場合、記法を使う時は演算子のような関数のときか、初期化子を作るときなのではないかと思います。
この内、演算子のような記法はすなわち 3 f 2 --> f 3 2 のような変換なわけですが、3 f 2 f 5と書かれたときの解釈の仕方として、
結合方向を決定しなければならないという問題があります。

3 f 2 f 5  ---> f 3 (f 2 5) -or- (f (f 3 2) 5)

規則は、A f B ---> f (A) (B)のような形で書かれます。変換後は関数のような形をしてなければなりません。
これは一種の機能制限ですが、これを許してしまうとかなり変な書き方になります。
基本的には関数で出来るので、記法の改造も関数のシノニムに過ぎないとしても大丈夫だと踏んでます。（名前結合子の存在は？）

まぁこれはそういうこととして、話を戻します。A f B ---> f (A) (B)変換があった時、
3 f 2 f 5
はどのように変換されるべきでしょうか。
前から変換していくと、

,A f B, --> f (A) (B)

,(|x y| ,x f y,) f (3 f 2),

lexer: [Byte] -> [#Symbol]
parser: [#Symbol] -> (a -> a)
intprt: (a -> a) -> (a -> a) /World
notation: [#Symbol] (a -> a) (a -> a) -> (a -> a)

<結合性とnotation>
(As f Bs) = (f As Bs)
という規則があったら、現行の実装(gnotation)では
3 f 5 f 7 f 2 は
3 f (5 f 7 f 2)
f 3 (5 f 7 f 2)
f 3 (f 5 (7 f 2))
f 3 (f 5 (f 7 2))
というように変換される
3 f 5 f 7 f 2 は
3 f (5 f 7 f 2)
f (5 f 7 f 2) 3
f (f (7 f 2) 5) 3
f (f (f 2 7) 5) 3

notationは
(As f Bs) = (f As Bs)
という規則があったら
3 f 5 f 7 f 2 は
(3 f 5) f 7 f 2
(f 3 5) f 7 f 2
(f (f 3 5) 7) f 2
f (f (f 3 5) 7) 2
というように変形される


(notation (A = B ; C) ((|A| C) (B)) (
     (notation (A ; B) ((|x| B) (A)) (
    c = 8;
    add c 7;
    add 3 2;
    add 4 c
))))

(notation (A ; B) ((|x| B) (A)) (
    add c 7;
    add 3 2;
    add 4 c
))

(notation (As $ Bs) (As (Bs)) (
(notation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(notation (As ; Bs) ((|x| Bs) (As)) (
    let c = (|x y| add x y) 2 (negate 3);
    add 3 2;
    add 2 c
))
))
))



let c = 3 ; add c 7 ; add 3 2 ; add 4 c
(|c| (|x| (|x| add 4 c)) 3

(|x| B) (A)

(notation (PREDICTs ? TRUEs else FALSEs) ((fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs) (
(notation (As == Bs) ((fuse (|Bs| 1) (|otherwise| 0)) As) (
          3 == 3 ? yes else no          
))
))



(gnotation (notation As =>> Bs ; Cs) (notation As Bs Cs) (
(gnotation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (As ; Bs) ((|x| Bs) (As)) (
    notation PREDICTs ? TRUEs else FALSEs =>> (fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs;
    notation As == Bs =>> (fuse (|Bs| 1) (|otherwise| 0)) As;
    notation As $ Bs =>> As Bs;
    notation As + Bs =>> add As Bs;
    notation As - Bs =>> subtract As Bs;

    let subtract = (|a b|
        add a $ negate b
    );
    let mul = (|a b|
        b == 0 ?
            1
        else
            add a $ mul a (b - 1)
    );

    mul 4 5
))
))
))

(gnotation (notation As =>> Bs ; Cs) (notation As Bs Cs) (
(gnotation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
    notation PREDICTs ? TRUEs else FALSEs =>> (fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs;
    notation As == Bs =>> (fuse (|Bs| 1) (|otherwise| 0)) As;
    let mul = (|a b| b == 0 ? 0 else add a (mul a (add b (negate 1))));
    mul 4 5
))
))

(notation (PREDICTs ? TRUEs else FALSEs) ((fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs) (
(notation (As == Bs) ((fuse (|Bs| 1) (|otherwise| 0)) As) (
          (|mul| mul 7 8) (|a b| b == 0 ? 0 else add a (mul a (add b (negate 1))))
))
))

(notation (PREDICTs ? TRUEs else FALSEs) ((fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs) (
(notation (As == Bs) ((fuse (|Bs| 1) (|otherwise| 0)) As) (
          3 == 3 ? yes else no          
))
))


(gnotation (notation As =>> Bs ; Cs) (notation As Bs Cs) (
(gnotation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (As ; Bs) ((|x| Bs) (As)) (
    notation PREDICTs ? TRUEs else FALSEs =>> (fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs;
    notation As == Bs =>> (fuse (|Bs| 1) (|otherwise| 0)) As;
    notation As + Bs =>> add As Bs;
    notation As - Bs =>> subtract As Bs;
    notation As * Bs =>> multiplicate As Bs;
    notation As $ Bs =>> As Bs;

    let subtract = (|a b|
        add a $ negate b
    );
    let multiplicate = (|a b|
        b == 0 ?
            0
        else
            add a $ multiplicate a (b - 1)
    );
    let mulWord = (|a b|
        b == 0 ?
            .
        else
            concat a $ mulWord a (b - 1)
    );

    let fib = (|n|
        n == 1 ? 1
        else (
            n == 2 ? 1
            else fib (n - 1) + fib (n - 2)
        )
    );

    fib 6
))
))
))



心配なコード

(|f| f 2 3) add

(|f| f 3)
(fuse (|2| 1) (|otherwise| 0))
(gnotation (notation As =>> Bs ; Cs) (notation As Bs Cs) (
(gnotation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (As ; Bs) ((|x| Bs) (As)) (
    notation PREDICTs ? TRUEs else FALSEs =>> (fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs;
    notation As == Bs =>> (fuse (|Bs| 1) (|otherwise| 0)) As;
    notation As + Bs =>> add As Bs;
    notation As - Bs =>> subtract As Bs;
    notation As * Bs =>> multiplicate As Bs;
    notation As $ Bs =>> As Bs;

    let subtract = (|a b|
        add a $ negate b
    );
    let multiplicate = (|a b|
        b == 0 ?
            0
        else
            add a $ multiplicate a (b - 1)
    );
    let mulWord = (|a b|
        b == 0 ?
            .
        else
            concat a $ mulWord a (b - 1)
    );

    let fib = (|n|
        n == 1 ? 1
        else (
            n == 2 ? 1
            else fib (n - 1) + fib (n - 2)
        )
    );

    
    p
))
))
))

(gnotation (notation As =>> Bs ; Cs) (notation As Bs Cs) (
(gnotation (let A: TYPEs = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (As ; Bs) ((|x| Bs) (As)) (
    notation PREDICTs ? TRUEs else FALSEs =>> (fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs;
    notation As == Bs =>> (fuse (|Bs| 1) (|otherwise| 0)) As;
    notation As + Bs =>> add As Bs;
    notation As - Bs =>> subtract As Bs;
    notation As * Bs =>> multiplicate As Bs;
    notation As $ Bs =>> As Bs;

    let subtract = (|a b|
        add a $ negate b
    );
    let multiplicate = (|a b|
        b == 0 ?
            0
        else
            add a $ multiplicate a (b - 1)
    );
    let mulWord = (|a b|
        b == 0 ?
            .
        else
            concat a $ mulWord a (b - 1)
    );

    let fib = (|n|
        n == 1 ? 1
        else (
            n == 2 ? 1
            else fib (n - 1) + fib (n - 2)
        )
    );

    
    p
))
))
))

let multiplicate = (|a b|

    notation PREDICTs ? TRUEs else FALSEs =>> (fuse (|1| TRUEs) (|otherwise| FALSEs)) PREDICTs;
    notation As == Bs =>> (fuse (|Bs| 1) (|otherwise| 0)) As;
    notation As + Bs =>> add As Bs;
    notation As - Bs =>> subtract As Bs;
    notation As * Bs =>> multiplicate As Bs;
    gnotation As $ Bs =>> As Bs;

        b == 0 ?
            0
        else
            add a $ multiplicate a (b - 1)
    );
    let mulWord = (|a b|
        b == 0 ?
            .
        else
            concat a $ mulWord a (b - 1)
    );

    let fib = (|n|
        n == 1 ? 1
        else (
            n == 2 ? 1
            else fib (n - 1) + fib (n - 2)
        )
    );

    
    p

(|sub|

(|sub|

    (|ass| ass 5)
    (|n|
    (fuse
        (|0| 0)
        (|x| add x (ass (sub x 1)))) n
    )
)
(|a b| add a (negate b))

(|acc| acc 5) (|f| (fuse (|1| 1) (|x| add x (acc (sub x 1)))) f)
