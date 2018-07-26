<?php

$node = node_load(23);
unset($node->uuid);

node_save($node);