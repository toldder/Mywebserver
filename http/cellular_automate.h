#ifndef __CELLULAR_AUTOMATE_H__
#define __CELLULAR_AUTOMATE_H__

#include<vector>
#include<time.h>
#include<sstream>
#include<iostream>

class cellAutomate
{
public:
    cellAutomate(int n, int m) :grid(n, std::vector<bool>(m,false))
    {
        srand(time(0));
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < m; ++j)
            {
                double d = (double)rand() / RAND_MAX;
                if (d > 0.5) grid[i][j] = 1;
                else grid[i][j] = 0;
            }
        }
    }

    std::string cellState(int t) //返回时间t的状态，时间0是原始状态
    {
        int old_t = t;
        int N = grid.size(), M = grid[0].size();
        std::vector<std::vector<bool>> backup = grid;
        std::ostringstream os;

        //输出原始状态
        os << "时间t=0" << std::endl;
        for (int i = 0;i < N;++i)
        {
            for (int j = 0;j < M;++j)
            {
                os << grid[i][j] << " ";
            }
            os << std::endl;
        }

        while (t--)
        {
            os << "时间t=" << old_t - t << std::endl;
            backup = grid;
            for (int i = 0; i < N; ++i)
            {
                for (int j = 0; j < M; ++j)
                {
                    int alive = 0;
                    //统计周围细胞存活数量
                    for (int k = 0; k < 8; ++k)
                    {
                        int x = i + dx[k], y = j + dy[k];
                        if (x < 0 || y < 0 || x >= N || y >= M) continue; //非法的区间
                        if (backup[x][y]) alive++;
                    }
                    //如果周围存在2或3个细胞，状态为存活状态
                    if (alive == 2 || alive == 3) grid[i][j] = true;
                    else grid[i][j] = false; //否则，是死亡状态
                    os << grid[i][j]<<" ";
                }
                os << std::endl;
            }
        }
        return os.str();
    }
private:
    std::vector<std::vector<bool>> grid;
    const int dx[8] = { -1,1,0,0,-1 ,-1,1,1 };
    const int dy[8] = { 0,0,-1,1,-1,1,-1,1 }; //记录方向移动，上下左右，左上、右上、左下、右下
};

#endif // !__CELLULAR_AUTOMATE_H__