(gnotation (notation As =>> Bs ; Cs) (notation (As) Bs Cs) (
(gnotation (gnotation As =>> Bs ; Cs) (gnotation (As) Bs Cs) (
(gnotation (let A: TYPEs = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (As ; Bs) ((|x| Bs) (As)) (
    gnotation As $ Bs =>> As Bs;
    notation A - Bs   =>> subtract A Bs;

    let subtract = (|a b|
        add a $ negate b
    );

    let a = -3;
    a
))
))
))
))))