
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
            0
        else
            add a $ mul a (b - 1)
    );

    mul 4 5
))
))
))
