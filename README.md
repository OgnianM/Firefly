<h1> Firefly </h1>

A UCI-compliant neural network based chess engine.

Some of its roots lie in the Leela Chess Zero project, whose weights were used during development.

In fact, it still depends on LC0 weights to even be functional, training a network especially for Firefly is a future goal.

[Some technical details](Docs/Technical.pdf)

<h3> Dependencies </h3>

[libtorch](https://pytorch.org/get-started/locally) possibly with CUDA and cuDNN for use with NVIDIA GPUs.

[cmake](https://cmake.org/)

[protobuf](https://developers.google.com/protocol-buffers)


<h3> Building </h3>

<p><code> git clone --recursive https://github.com/OgnianM/Firefly </code> </p>
<p><code> mkdir Firefly/build </code> </p>
<p><code> cd Firefly/build </code> </p>
<p><code> cmake .. -DCMAKE_PREFIX_PATH=/path/to/libtorch/share/cmake/Torch </code> </p>
<p><code> make </code> </p>

<h3> Usage </h3>
<p>In the most basic case, it can be invoked with just a NN file like so <code> Firefly -n /path/to/nn.pb</code> </p>
<p> More options can be accessed with <code> Firefly -h </code> </p>

<h3> Status </h3>

<p> The engine is very early in development, there are many technical details that still need to be ironed out, 
as well as substantial performance issues and probably bugs. </p>

<h3> Strength </h3>

Not properly tested.

[Some games vs Stockfish 16T  nn-13406b1dcbe0.nnue](Docs/FireflyVStockfish_10g.pgn)

<p>Sometimes it manages to hold a draw, sometimes its position gradually falls apart, 
sometimes it blunders, pretty inconsistent overall. </p>