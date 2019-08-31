(notation (As $ Bs) (As (Bs)) (
(notation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(notation (As ; Bs) ((|x| Bs) (As)) (
    let c = (|x y| add x y) 2 (negate 3);
    add 3 2;
    add 2 c
))
))
))

