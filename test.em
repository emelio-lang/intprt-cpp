(|fact:(int->int)| fact 10) (fuse  (|0:int| 1) (|x:int| mul x (fact (sub x 1))))
