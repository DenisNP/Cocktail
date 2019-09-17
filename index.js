/* eslint-disable no-console */
const express = require('express');

const app = express();
const port = 3000;

let current = '5|100|100|100|0';
app.get('/set', (req, res) => {
    current = req.query.value;
    res.send(`Set to: ${current}`);
});
app.get('/current', (req, res) => res.send(current));

app.listen(port, () => console.log(`Listening on port ${port}!`));
