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

    mulWord hello 4
))
))
))
