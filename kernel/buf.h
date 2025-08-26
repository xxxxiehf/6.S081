struct buf {
    int valid;  // has data been read from disk?
    int disk;   // does disk "own" buf?
    uint dev;
    uint blockno;
    struct sleeplock lock;
    uint refcnt;
    uint tick;
    struct buf *prev;
    struct buf *next;
    uchar data[BSIZE];
};
