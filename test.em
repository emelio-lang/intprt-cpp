type Vec2 :(x:int y:int) (
type Vec3 :(x:int y:int z:int) (
     (|square:(int->int)| 
      (|norm2:((Vec2|Vec3)->int)|
       norm2 (Vec2 3 5))
      (fuse
       (|v:Vec2| add (square (_get v x)) (square (_get v y)) )
       (|v:Vec3| add (add (square (_get v x)) (square (_get v y))) (square (_get v z)))
       )
      ) (|n:int| mul n n)
)
)
