tnotation ([A])
type List :((car:int cdr:List)|*Nil) (
     (|foo:(List->int)|
     foo (:List 3 (:List Nil))
     ) (fuse (|x:(car:int cdr:List)| _get x car) (|x:(*Nil)| 0))
)


