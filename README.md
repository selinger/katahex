***Engine based on Katago TensorRT+GraphSearch***   
# Hex棋盘吃子棋   
Hex棋盘：每个点有6气（普通围棋是4气）   
吃子棋：先吃n个子就获胜，不允许pass（也就是最后会填死）   
由于吃子棋先手有明显优势，所以需要引入贴目
贴目：   
komi=0.5 双方都不允许pass   
komi=0.5+n 白棋允许pass n次   
komi=0.5-n 黑棋允许pass n次   
komi=0.0+n 白棋允许pass n次，但如果取胜时恰好pass了n次则判和棋   
komi=1.0-n 黑棋允许pass n次，但如果取胜时恰好pass了n次则判和棋   
以上贴目方法保证与还棋头点目等价   
   
还在跑，暂时还没有结论   