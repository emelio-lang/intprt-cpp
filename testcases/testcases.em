計算
add 2 3
5

全体が関数
(add 2 3)
5

冗長な関数
add (2) (3)
5

冗長すぎ
add ((2)) (((3)))
5

全体が冗長すぎ
((((add 2 3))))
5

冗長な関数 + 全体が関数
(add (2) (3))
5

即時関数
(|x| add x 2) 3
5

即時関数 + 全体関数
((|x| add x 2) 3)
5

引数に冗長な関数
(|x| add (x) 2) 3
5

引数に冗長すぎな関数
(|x| add ((x)) 2) 3
5

関数のボディが冗長な関数
(|x| (add x 2)) 3
5

関数のボディが冗長 + 全体関数
((|x| (add x 2)) 3)
5

全部冗長
(((|x| ((add ((x)) (2)))) ((3))))
5

複数値関数
(|x y| add x y) 2 3
5

全体 + 複数
((|x y| add x y) 2 3)
5

多重関数
(|x1 y1 z1 w1| add w1 ((|x2 y2 z2| add z2 ((|x3 y3| add x3 y3) x2 y2)) x1 y1 z1)) 10 30 24 1
65

シャドウ
(|x y z w| add w ((|x y z| add z ((|x y| add x y) x y)) x y z)) 10 30 24 1
65

引数に関数評価が必要
(|x y| (|u| add u 3) x) ((|a| 1) 3) 5
4

引数と名前の衝突
(|x y| add x y) ((|x| negate x) 3) ((|x| x) 5)
2

引数関数の複数回適用
(|subtract| subtract (subtract 2 7) 8) (|a b| add a (negate b))
-13

空引数
(|f| f 1 2) ((|x _ y| add x y) 3)
5
