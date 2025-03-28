import fs from "node:fs";

const trim = (s: string) => {
  while (true) {
    const n = s.replaceAll("  ", " ");
    if (n === s) {
      return n;
    }
    s = n;
  }
};

const consume = (lines: string[], start: number): number => {
  const first = lines[start];
  if (!first.startsWith("DEF_LOOKUP")) {
    throw new Error();
  }
  const tokens = trim(first).split(" ");
  tokens.shift();
  const name = tokens.shift();
  let base: string;
  switch (tokens.shift()) {
    case "PROCESS_BASE":
      base = "processBase()";
      break;
    case "SKIP_BASE":
      base = "skipBase()";
      break;
    default:
      throw new Error();
  }
  let marks: string;
  switch (tokens.shift()) {
    case "PROCESS_MARKS": {
      const marksWhat = tokens.shift();
      if (marksWhat === "MARK_GLYPH_SET") {
        marks = `processMarkGlyphs(${tokens.shift()})`;
      } else if (marksWhat === "ALL" || marksWhat === `"ALL"`) {
        marks = "processMarksAll()";
      } else if (marksWhat?.startsWith(`"`)) {
        marks = `processMarkGroup(${marksWhat})`;
      } else {
        throw new Error();
      }
      break;
    }
    case "SKIP_MARKS":
      marks = "skipMarks()";
      break;
    default:
      throw new Error();
  }
  if (tokens.shift() !== "DIRECTION") {
    throw new Error();
  }
  const directionWhat = tokens.shift();
  if (directionWhat !== "LTR") {
    throw new Error();
  }
  console.log(`defineLookup(${name})->${base}->${marks}`);
  for (let i = start + 1; i < lines.length; i++) {
    const l = lines[i];
    if (l.endsWith("END_SUBSTITUTION") || l.endsWith("END_POSITION")) {
      console.log(`->end();`);
      return i + 1;
    }
  }
  return lines.length;
};

const main = async () => {
  const vtp = (await fs.promises.readFile("EgyptianText_200.vtp")).toString("utf-8");
  const lines = vtp.split("\n");
  for (let i = 0; i < lines.length;) {
    const l = lines[i];
    if (l.startsWith("DEF_LOOKUP")) {
      i = consume(lines, i);
    } else {
      i += 1;
    }
  }
};

main();
