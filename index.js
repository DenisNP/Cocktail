/* eslint-disable no-console */
const express = require('express');

const app = express();
const port = 3000;

app.get('/current', (req, res) => res.send('5|100|100|100|0'));

app.listen(port, () => console.log(`Listening on port ${port}!`));
