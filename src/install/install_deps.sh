RDMA_REQ="rdma-core rdmacm-utils librdmacm-dev libx11-dev xcb-proto libxcb-xfixes0-dev libxtst-dev"
RDMA_OPT="perftest infiniband-diags"

if []

while [ "$REPLY" != "y" ] && [ "$REPLY" != "n" ]
do
    echo -n "Update system before install? (y/n): "
    read
done

sudo apt update

if [ "$REPLY" = "y" ]
then
    apt -y upgrade
fi

while [ "$REPLY" != "y" ] && [ "$REPLY" != "n" ]
do
    echo -n "Install optional RDMA libraries? (y/n): "
    read
done

if [ "$REPLY" = "y" ]
then
    echo "Installing optional libraries."
    apt install -y $RDMA_REQ $RDMA_OPT
else
    echo "Skipping optional install."
    apt install -y $RDMA_REQ
fi