Let Notation (;もあるよ)
5
(gnotation (let A = As ; Bs) ((|A| Bs) (As)) (
(gnotation (As ; Bs) ((|x| Bs) (As)) (
  let subtract = (|a b| add a (negate b));
  add 2 3;
  let c = subtract 2 4;
  let d = add 7 c;
  add c 4;
  d
))
))