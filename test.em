(gnotation (notation As = Bs ; Cs) (notation As Bs Cs) (
(gnotation (let A = Bs ; Cs) ((|A| Cs) (Bs)) (
(gnotation (As ; Bs) ((|x| Bs) (As)) (
  notation As $ Bs = As Bs;
  let subtract = (|a b|
      add a $ negate b
  );

  notation As + Bs = add As Bs;
  notation As - Bs = subtract As Bs;

  let u = 3 - 7 + 2;

  3 - 5 + 2 - 8 - 7 + u
))
))
))
