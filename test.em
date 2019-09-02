//hello
(gnotation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (As ; Bs) ((|x| Bs) (As)) (
(gnotation (As $ Bs) (As Bs) (
  let subtract = (|a b|
      add a $ negate b
  );

  notation As + Bs = add As Bs;
  notation As - Bs = subtract As Bs;

  let u = 3 - 5;
  let v = 2 - 8 - 7;

  3 - 5 + 2 - 8 -7 + 13
))
))
))
